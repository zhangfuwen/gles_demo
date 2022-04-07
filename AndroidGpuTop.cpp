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

bool g_textMode = false;

//double to_double(struct timeval a) {
//    auto ret = (double)a.tv_sec + ((double )a.tv_usec /1000000);
//    return ret;
//}
//
//struct timeval from_double(double time) {
//    long sec = time;
//    return timeval {
//        .tv_sec = sec,
//        .tv_usec = (long)((time - sec)*1000000),
//    };
//}

int64_t to_usecs(struct timeval a) {
    auto ret = (int64_t)a.tv_sec * 1000000 + a.tv_usec;
    return ret;
}

struct timeval from_usecs(int64_t val) {
    return timeval {
        .tv_sec = val /1000000,
        .tv_usec = val % 1000000,
    };
}

bool operator> (const timeval & a, const timeval &b) {
    return to_usecs(a) > to_usecs(b);
}
bool operator< (const timeval & a, const timeval &b) {
    return to_usecs(a) < to_usecs(b);
}

struct timeval operator-(const struct timeval & a, const struct timeval & b ) {
    auto diff = to_usecs(a) - to_usecs(b);
    return from_usecs(diff);
}
struct timeval operator+(const struct timeval & a, const struct timeval & b ) {
    auto sum = to_usecs(a) + to_usecs(b);
    return from_usecs(sum);
}

struct TimeRange {
    struct timeval start;
    struct timeval end;

    bool InRange (const struct timeval val) const {
        return val > start && val < end;
    }

    bool IntersectWith(TimeRange b) const {
        return InRange(b.start) || InRange(b.end);
    }

    struct timeval Substract(TimeRange b) const {
        if (InRange(b.start)) {
            if (InRange(b.end)) {
                return (b.start - start) + (end - b.end);
            } else {
                return b.start - start;
            }
        } else if (InRange(end)) {
            return end -b.end;
        } else {
            return end -start;
        }

    }

    struct TimeRange ClipTo(struct TimeRange b) {
        return { b.start > start ? b.start: start,
                 b.end < end? b.end: end};
    }
};

int SearchFallsInRange(const std::vector<TimeRange> & ranges, int i1, int i2, struct timeval a) {
    if(ranges.empty()) {
        return -1;
    }
    if(i1 == i2) {
        if(ranges[i1].InRange(a)) {
            return i1;
        } else {
            return -1;
        }
    }
    int middle = (i1 + i2)/2;
    if(auto ret = SearchFallsInRange(ranges, i1, middle, a) ; ret != -1) {
        return ret;
    }
    if(auto ret = SearchFallsInRange(ranges, middle+1, i2, a) ; ret != -1) {
        return ret;
    }
    return -1;
}

std::pair<int, int> SearchIntersects(std::vector<TimeRange> ranges, TimeRange a) { // may have many
    int i1 = SearchFallsInRange(ranges,0, ranges.size() -1,  a.start);
    if(i1 == -1) {
        return { -1, -1};
    }
    int i2 = SearchFallsInRange(ranges, i1, ranges.size() -1, a.end);
    if(i2 == -1) {
        for(int i = i1; i < ranges.size(); i++) {
            if(a.end > ranges[i].start) {
                i2 = i;
            }
        }
        if(i2 >= 0) {
            return {i1, i2};
        } else {
            return { -1, -1};
        }
//        return {i1, i1};
    } else {
        return {i1, i2};
    }
}

struct Record {
    std::string threadName;
    int ctx; // context id
    int ts; // transaction id?
    int seq;
    int sec;
    int usec;
    struct TimeRange timeRange;
    int start;// ticks
    int retire;// ticks
};


struct PreemptRecord {
    struct TimeRange timeRange;
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
const int ticksPerSecond = 19200000;

struct Top {
private:
    int cur_second = 0;
//    std::map<int, int> ctxTicks;// context id, to ticks
    std::vector<Record> records;
    std::vector<PreemptRecord> preempts;

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
//        ctxTicks[record.ctx] += record.retire - record.start;
        records.emplace_back(record);
    }
    void AddPreemptRecord(PreemptRecord preemptRecord) {
        preempts.emplace_back(preemptRecord);
    }
    void Reset(int newSec) {
        cur_second = newSec;
//        ctxTicks.clear();
        records.clear();
        preempts.clear();
    }
    void Print() {
        if(!g_textMode) {
            printf("\033c");
        }
        std::map<int, int64_t> ctxUsecs;// context id, to usecs
        std::vector<TimeRange> preemptRanges;
        int64_t totalPreempt = 0;
        for(const auto & preempt : preempts) {
            preemptRanges.emplace_back(preempt.timeRange);
            totalPreempt += to_usecs(preempt.timeRange.end - preempt.timeRange.start);
        }
        for(const auto & record : records) {
            auto p = SearchIntersects(preemptRanges, record.timeRange);
            auto recordTotal = to_usecs(record.timeRange.end -record.timeRange.start);
            if(recordTotal < 0 || recordTotal > 1000000) {
                FUN_INFO("");
            }
            if(p.first != -1) {
                for(int i = p.first ; i <= p.second; i++) {
                    auto pre = preemptRanges[i];
                    auto clip = pre.ClipTo(record.timeRange);
                    auto clipTime = to_usecs(clip.end - clip.start);
                    recordTotal -= clipTime;
                    if(recordTotal < 0 || recordTotal > 1) {
                        FUN_INFO("");
                    }
                }

            }
            if(!ctxUsecs.count(record.ctx)) {
                ctxUsecs[record.ctx] = 0;
            }
            ctxUsecs[record.ctx] += recordTotal;
            if(ctxUsecs[record.ctx] < 0 || ctxUsecs[record.ctx] > 1000000) {
                FUN_INFO("");
            }

        }

        for(const auto & [ctx, usecs]: ctxUsecs) {
            FUN_INFO("time:%d ctx:(%3d) ticks:(%2.2f%%) name:%s",
                     cur_second,
                     ctx,
                     ((double )usecs)/1000000 * 100,
                    ctxPname.count(ctx) ? ctxPname[ctx].c_str():""
            );
        }
        FUN_INFO("totalPreempt: %2.2f%%, %d", totalPreempt/1000000*100, preemptRanges.size());
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

        int64_t tick_diff = retire - start;
        int64_t timediff_usecs = tick_diff / (ticksPerSecond/1000000);
        int64_t endTimeUsecs = sec* 1000000 + usec;
        int64_t startTimeUsecs = endTimeUsecs - timediff_usecs;

        r.timeRange = {
                .start = from_usecs(startTimeUsecs),
                .end = {
                    .tv_sec = sec,
                    .tv_usec = usec,
                }
        };
        if(r.timeRange.end < r.timeRange.start) {
            FUN_INFO("xx");
        }
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

int preemptProcessing(std::string line, Top & top)
{
    using namespace handycpp::string::pipe_operator;
    using namespace handycpp::dyntype::arithmetic;

    auto match = line | grep("adreno_preempt_");
    if(match.has_value()) {
        auto pname = handycpp::string::trim_copy(line.substr(0, 23));
        auto time = line.substr(34, 13);
        auto rest = line.substr(49);
        auto cmd = rest.substr(0, rest.find_first_of(":"));
        auto tok2 = time | splitby(".");
        auto sec = 0 + tok2[0];
        auto usec = 0 + tok2[1];
        static long long start_sec;
        static long long start_usec;
        if(cmd == "adreno_preempt_trigger") {
            start_sec = sec;
            start_usec = usec;
        } else if (cmd == "adreno_preempt_done") {
            top.AddPreemptRecord({
                .timeRange = {
                    .start = {
                        .tv_sec = start_sec,
                        .tv_usec = start_usec,
                    },
                    .end = {
                        .tv_sec = sec,
                        .tv_usec = usec,
                    }
                }
            });

        }

        return 0;
    } else {
        return -1;
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

    g_textMode = args.get<bool>("b").value();

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
        if(auto x = preemptProcessing(line, top); x < 0) {
            //
        }

        if(auto x = line | grep("adreno_cmdbatch_submitted:"); x.has_value()) {
            submitProcessing(line, top);
        }

        if(auto x = line | grep("retired:"); x.has_value()) {
            auto rec = retireProcessing(line);
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
