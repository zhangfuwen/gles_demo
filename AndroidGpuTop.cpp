//
// Created by zhangfuwen on 2022/3/23.
//

#include "AndroidGpuTop.h"
#define FUN_PRINT(fmt, ...) printf(fmt"\n", ##__VA_ARGS__); fflush(stdout)
#include "handycpp/logging.h"
#include "handycpp/string.h"
#include "handycpp/file.h"
#include "handycpp/dyntype.h"
#include <map>
#include <chrono>

#include "handycpp/time.h"
#include "handycpp/exec.h"

struct Record {
    std::string threadName;
    int ctx; // context id
    int ts; // transaction id?
    int seq;
    int sec;
    int usec;
    int start;// ticks
    int retire;// ticks
};

#ifdef ANDROID

void setFileEnabled(std::string Path, bool enabled) {
    using namespace std;
    std::string s = enabled? "1" : "0";
    handycpp::file::saveFile(s.data(), 2, Path); // 好像要写\0才行
}

std::string getFileContent(std::string Path) {
    auto ret = handycpp::file::readText(Path);
    return ret;
}

bool getFileEnabled(std::string Path) {
    auto x = getFileContent(Path);
    x = handycpp::string::trim(x);
    if(x == "0") {
        return false;
    }
    if(x == "1") {
        return true;
    }
    FUN_ERROR("error: got (%d)%s for %s", x.size(), x.c_str(), Path.c_str());
    return false;
}

#endif

struct Top {
private:
    const int ticksPerSecond = 19200000;
    int cur_second = 0;
    std::map<int, int> ctxTicks;// context id, to ticks

    std::map<int, std::string> ctxPname;

public:

    void UpdateCtxPnameMap(int ctx, std::string pname) {
        ctxPname[ctx] = pname;
    }

    void AddRecord(Record record) {
        if(cur_second != record.sec) {
            Print();
            Reset(record.sec);
        }
        ctxTicks[record.ctx] += record.retire - record.start;
    }
    void Reset(int newSec) {
        cur_second = newSec;
        ctxTicks.clear();
    }
    void Print() {
        printf("\033c");
        for(const auto & [ctx, ticks]: ctxTicks) {
            FUN_INFO("time:%d ctx:(%3d) ticks:(%2.2f%%) name:%s",
                     cur_second,
                     ctx,
                     ((float)ticks)/ticksPerSecond * 100,
                    ctxPname.count(ctx) ? ctxPname[ctx].c_str():""
            );
        }
    }
};

std::optional<Record> retireProcessing(std::string line) {
    using namespace handycpp::string::pipe_operator;
    using namespace handycpp::dyntype::arithmetic;

    auto match = line | egrep_submatch("\\ *([a-zA-Z0-9\\-_\\. ]{21})\\ *\\[([0-9]*)\\]\\ .{4} *([0-9]{0,6})\\.([0-9]{6}):\\ adreno_cmdbatch_retired:(.*)$");
//    FUN_INFO("match.size:%d", match.size());
    if(match.size()!=0){
        std::string thread_name = match[1];
        int seq = 0 + match[2];
        int sec = 0 + match[3];
        int usec = 0 + match[4];
        int start = 0; // ticks
        int retire = 0; // ticks
        std::string msg = match[5];
        auto records = msg | split;
        std::map<std::string, std::string> m;
        for(const std::string rec : records) {
            auto kv = rec | splitby("=");
            kv[1] = handycpp::string::ReplaceSuffix(kv[1], ",","");
            m[kv[0]] = kv[1];
        }
        start = 0 + m["start"];
        retire = 0 + m["retire"];
        Record r;
        r.threadName = thread_name;
        r.seq = seq;
        r.sec = sec;
        r.usec = usec;
        r.start = start;
        r.retire = retire;
        r.ctx = 0 + m["ctx"];
        r.ts = 0 + m["ts"];
        return r;
    }
    return std::nullopt;
}


void submitProcessing(std::string line, Top & top)
{
    using namespace handycpp::string::pipe_operator;
    using namespace handycpp::dyntype::arithmetic;

    auto match = line | egrep_submatch("\\ *([a-zA-Z0-9\\-_\\. ]{21})\\ *\\[([0-9]*)\\]\\ .{4} *([0-9]{0,6})\\.([0-9]{6}):\\ adreno_cmdbatch_submitted:(.*)$");
    if(match.size()!=0){
        std::string thread_name = match[1];
        std::string msg = match[5];
        auto records = msg | split;
        for(const std::string rec : records) {
            auto kv = rec | splitby("=");
            if(kv[0] == "ctx") {
                top.UpdateCtxPnameMap(0 + kv[1], thread_name);
            }
        }
    }
}

#include "handycpp/flags.h"


using namespace std;
#ifdef ANDROID
std::array eventFiles = {
        "/sys/kernel/debug/tracing/events/kgsl/adreno_cmdbatch_retired/enable"s,
        "/sys/kernel/debug/tracing/events/kgsl/adreno_cmdbatch_submitted/enable"s,
        "/sys/kernel/debug/tracing/events/kgsl/adreno_preempt_done/enable"s,
        "/sys/kernel/debug/tracing/events/kgsl/adreno_preempt_trigger/enable"s
};
#endif



#include <signal.h>
volatile bool stop = false;
int main(int argc, char** argv) {
    const flags::args args(argc, argv);

#ifdef ANDROID
    std::string filePath = "/sys/kernel/debug/tracing/trace_pipe";
#else
    std::string filePath = "./AndroidGpuTop.log";
#endif
    const auto res = args.get<std::string>("f");
    if(res.has_value()) {
        filePath = res.value();
    }
    FUN_INFO("filename: %s -", filePath.c_str());

    int timeout = 0;
    if(auto to = args.get<int>("t"); to.has_value()) {
        timeout = to.value();
    }
    FUN_INFO("timeout: %d seconds", timeout);

    handycpp::time::timer timer;
    using namespace std::chrono_literals;
    if(timeout != 0) {
        timer.setTimeout([](){
            stop = true;
        }, 1s * timeout);
    }

    signal(SIGINT, [](int no) {
        FUN_INFO("sig int caught");
        stop = true;
    });
    signal(SIGTERM, [](int no) {
        FUN_INFO("sig term caught");
        stop = true;
    });


#ifdef ANDROID
    // backup event status and set enabled
    std::map<std::string, bool> eventFileStatueBackup;
    for(const auto & eventFile : eventFiles) {
        eventFileStatueBackup[eventFile] = getFileEnabled(eventFile);
        FUN_INFO("backup %s -> %d", eventFile.c_str(), eventFileStatueBackup[eventFile]);
        setFileEnabled(eventFile, true);
    }
    setFileEnabled("/sys/kernel/debug/tracing/tracing_on", true);
#endif

    // process lines and print
    using namespace handycpp::string::pipe_operator;
    using namespace handycpp::dyntype::arithmetic;
    Top top;
    auto numLines = handycpp::file::for_each_line(filePath, [&top](int i, std::string line) -> int {
        if(stop) {
            FUN_INFO("stopping, line:%d",i );
            return -1;
        }

        if(auto x = line | grep("adreno_cmdbatch_submitted:"); x.has_value()) {
            ctxPnameProcessing(line, top);
        }

        if(auto x = line | grep("retired:"); x.has_value()) {
            auto rec = lineProcessing(line);
            if(x.has_value() && !rec.has_value()) {
//                FUN_DEBUG("not matching (%d):%s", line.size(), line.c_str());
            }
            if(rec.has_value()) {
                top.AddRecord(rec.value());
            }
        }

        return 0;

    });
    FUN_INFO("parsed lines:%d", numLines);

#ifdef ANDROID
    // restore event status
    for(const auto & [file, enablement] : eventFileStatueBackup) {
        setFileEnabled(file, enablement);
    }
    setFileEnabled("/sys/kernel/debug/tracing/tracing_on", false);

    // TODO: delete this
//    for(const auto & [file, enablement] : eventFileStatueBackup) {
//        setFileEnabled(file, true);
//        bool ret = getFileEnabled(file);
//        FUN_INFO("set false: return %d", ret);
//    }

#endif

    return 0;
}
