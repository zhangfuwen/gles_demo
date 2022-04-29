/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2021 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "arm_counters.h"

#include <dlfcn.h>

#define FUN_INFO(fmt, ...)                                                                                             \
    printf(fmt "\n", ##__VA_ARGS__);                                                                                   \
    fflush(stdout)
#define FUN_ERR(fmt, ...)                                                                                              \
    printf(fmt "\n", ##__VA_ARGS__);                                                                                   \
    fflush(stdout)

///
#if 1 // defined(__ANDROID__) || defined(ANDROID)
using namespace std;

using pfnGLGetPerfMonitorCounterStringAMD =
    void (*)(GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString);
using pfnGLGetPerfMonitorGroupStringAMD = void (*)(GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString);
using pfnGLGetPerfMonitorCountersAMD =
    void (*)(GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters);
using pfnGLGetPerfMonitorGroupsAMD = void (*)(GLint *numGroups, GLsizei groupsSize, GLuint *groups);
using pfnGLGenPerfMonitorsAMD = void (*)(GLsizei n, GLuint *monitors);
using pfnGLSelectPerfMonitorCountersAMD =
    void (*)(GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList);
using pfnGLBeginPerfMonitorAMD = void (*)(GLuint monitor);
using pfnGLEndPerfMonitorAMD = void (*)(GLuint monitor);
using pfnGLGetPerfMonitorCounterDataAMD =
    void (*)(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten);
using pfnGLGetPerfMonitorCounterInfoAMD = void (*)(GLuint group, GLuint counter, GLenum pname, void *data);
using pfnGLDeletePerfMonitorsAMD = void (*)(GLsizei n, GLuint *monitors);

namespace {
pfnGLGenPerfMonitorsAMD glGenPerfMonitorsAMD = nullptr;
pfnGLSelectPerfMonitorCountersAMD glSelectPerfMonitorCountersAMD = nullptr;
pfnGLBeginPerfMonitorAMD glBeginPerfMonitorAMD = nullptr;
pfnGLEndPerfMonitorAMD glEndPerfMonitorAMD = nullptr;
pfnGLGetPerfMonitorCounterDataAMD glGetPerfMonitorCounterDataAMD = nullptr;
pfnGLGetPerfMonitorCounterInfoAMD glGetPerfMonitorCounterInfoAMD = nullptr;
pfnGLDeletePerfMonitorsAMD glDeletePerfMonitorsAMD = nullptr;
pfnGLGetPerfMonitorCounterStringAMD glGetPerfMonitorCounterStringAMD = nullptr;
pfnGLGetPerfMonitorGroupStringAMD glGetPerfMonitorGroupStringAMD = nullptr;
pfnGLGetPerfMonitorGroupsAMD glGetPerfMonitorGroupsAMD = nullptr;
pfnGLGetPerfMonitorCountersAMD glGetPerfMonitorCountersAMD = nullptr;
} // namespace

CounterDescription g_counterDescs[] = {
    {
        .name = "% Time Shading Vertices",
        .counter = GPUCounter((int)GPUCounter::FirstQCOM + 1),
        .category = "Pico",
        .description = "% Time Shading Vertices",
        .resultByteWidth = 8,
        .resultType = CompType::Float,
        .unit = CounterUnit::Percentage,
    },
};

static CounterDescription
QCOMCreateCounterDescription(GPUCounter index, const std::string &groupName, const std::string &counterName) {
    CounterDescription desc;
    desc.name = counterName;
    desc.counter = GPUCounter(index);
    desc.category = groupName;
    desc.description = counterName;
    desc.resultByteWidth = 8;
    desc.resultType = CompType::UInt;
    desc.unit = CounterUnit::Absolute;
    return desc;
}

void QCOMCounters::GetGroupAndCounterList(GLuint **groupsList, int *numGroups, CounterInfo **counterInfo) {
    GLint n;
    GLuint *groups;
    CounterInfo *counters;

    glGetPerfMonitorGroupsAMD(&n, 0, NULL);
    groups = (GLuint *)malloc(n * sizeof(GLuint));
    glGetPerfMonitorGroupsAMD(NULL, n, groups);
    *numGroups = n;

    *groupsList = groups;
    // counters = (CounterInfo *)malloc(sizeof(CounterInfo) * n);
    counters = new CounterInfo[n];
    uint32_t idx = 1; // 注意从1开始
    for (int i = 0; i < n; i++) {
        glGetPerfMonitorCountersAMD(groups[i], &counters[i].numCounters, &counters[i].maxActiveCounters, 0, NULL);

        counters[i].counterList = (GLuint *)malloc(counters[i].numCounters * sizeof(int));

        glGetPerfMonitorCountersAMD(groups[i], NULL, NULL, counters[i].numCounters, counters[i].counterList);

        char curGroupName[256] = {0};
        glGetPerfMonitorGroupStringAMD(groups[i], 256, NULL, curGroupName);
        FUN_INFO("groupname[%d]: %s", i, curGroupName);

        counters[i].groupName = std::string(curGroupName);
        for (int j = 0; j < counters[i].numCounters; j++) {
            char curCounterName[256] = {0};

            glGetPerfMonitorCounterStringAMD(groups[i], counters[i].counterList[j], 256, NULL, curCounterName);

            FUN_INFO("countername[%d]: %s", j, curCounterName);
            counters[i].counterNames.push_back(std::string(curCounterName));

            MyCounter tmp;
            tmp.groupId = groups[i];
            tmp.counterId = counters[i].counterList[j];
            tmp.name = std::string(curCounterName);
            m_MyCounters.push_back(tmp);

            CounterDescription desc = QCOMCreateCounterDescription(
                GPUCounter((int)GPUCounter::FirstQCOM + idx), counters[i].groupName, counters[i].counterNames[j]);
            idx++; // 注意增加这个索引
            m_CounterDescriptions.push_back(desc);
            m_CounterIds.push_back(desc.counter);
        }
    }

    *counterInfo = counters;
}

bool QCOMCounters::Init() {
    FUN_INFO("QCOMCounters::Init");
    glGenPerfMonitorsAMD = (pfnGLGenPerfMonitorsAMD)eglGetProcAddress("glGenPerfMonitorsAMD");
    glSelectPerfMonitorCountersAMD =
        (pfnGLSelectPerfMonitorCountersAMD)eglGetProcAddress("glSelectPerfMonitorCountersAMD");
    glBeginPerfMonitorAMD = (pfnGLBeginPerfMonitorAMD)eglGetProcAddress("glBeginPerfMonitorAMD");
    glEndPerfMonitorAMD = (pfnGLEndPerfMonitorAMD)eglGetProcAddress("glEndPerfMonitorAMD");
    glGetPerfMonitorCounterDataAMD =
        (pfnGLGetPerfMonitorCounterDataAMD)eglGetProcAddress("glGetPerfMonitorCounterDataAMD");
    glGetPerfMonitorCounterInfoAMD =
        (pfnGLGetPerfMonitorCounterInfoAMD)eglGetProcAddress("glGetPerfMonitorCounterInfoAMD");
    glDeletePerfMonitorsAMD = (pfnGLDeletePerfMonitorsAMD)eglGetProcAddress("glDeletePerfMonitorsAMD");
    glGetPerfMonitorCounterStringAMD =
        (pfnGLGetPerfMonitorCounterStringAMD)eglGetProcAddress("glGetPerfMonitorCounterStringAMD");
    glGetPerfMonitorGroupStringAMD =
        (pfnGLGetPerfMonitorGroupStringAMD)eglGetProcAddress("glGetPerfMonitorGroupStringAMD");
    glGetPerfMonitorGroupsAMD = (pfnGLGetPerfMonitorGroupsAMD)eglGetProcAddress("glGetPerfMonitorGroupsAMD");
    glGetPerfMonitorCountersAMD = (pfnGLGetPerfMonitorCountersAMD)eglGetProcAddress("glGetPerfMonitorCountersAMD");

    if (glGenPerfMonitorsAMD == nullptr || glSelectPerfMonitorCountersAMD == nullptr ||
        glBeginPerfMonitorAMD == nullptr || glEndPerfMonitorAMD == nullptr ||
        glGetPerfMonitorCounterDataAMD == nullptr || glGetPerfMonitorCounterInfoAMD == nullptr ||
        glDeletePerfMonitorsAMD == nullptr || glGetPerfMonitorCounterStringAMD == nullptr ||
        glGetPerfMonitorGroupStringAMD == nullptr || glGetPerfMonitorGroupsAMD == nullptr ||
        glGetPerfMonitorCountersAMD == nullptr) {
        FUN_ERR("QCOM Counter init failed");
        return false;
    }

    GetGroupAndCounterList(&m_groups, &m_numGroups, &m_counters);
    FUN_INFO("QCOMCounter init success");
    return true;
}

void QCOMCounters::BeginPass(uint32_t passID) {
    m_passIndex = passID;
    // create perf monitor ID
    glGenPerfMonitorsAMD(1, &m_monitor);
    if (auto err = glGetError(); err != GL_NO_ERROR) {
        FUN_ERR("failed to GenPerfMonitors");
    }
    FUN_INFO("adsfad xx %d", m_EnabledCounters.size());
    m_EnabledCountersSupported.resize(m_EnabledCounters.size());
    FUN_INFO("adsfad");

    // enable the counters
    GLuint group;
    GLuint counter[1];
    for (size_t i = 0; i < m_EnabledCounters.size(); i++) {
        FUN_INFO("adsfad %d", i);
        group = m_MyCounters[m_EnabledCounters[i]].groupId;
        counter[0] = m_MyCounters[m_EnabledCounters[i]].counterId;
        FUN_INFO("select %d", i);
        glSelectPerfMonitorCountersAMD(m_monitor, GL_TRUE, group, 1, &counter[0]);
        FUN_INFO("end select %d", i);
        if (auto err = glGetError(); err != GL_NO_ERROR) {
            //            FUN_ERR("failed to Select %s",
            //            GetCounterDescription((GPUCounter)((uint32_t)m_EnabledCounters[i] +
            //            (uint32_t)GPUCounter::FirstQCOM)).name.c_str());
            m_EnabledCountersSupported[i] = false;
        } else {
            FUN_ERR(
                "success to Select %s",
                GetCounterDescription((GPUCounter)((uint32_t)m_EnabledCounters[i] + (uint32_t)GPUCounter::FirstQCOM))
                    .name.c_str());
            m_EnabledCountersSupported[i] = true;
        }
    }
}

void QCOMCounters::EndPass() {
    // disable the counters
    GLuint group;
    GLuint counter[1];
    for (size_t i = 0; i < m_EnabledCounters.size(); i++) {
        group = m_MyCounters[m_EnabledCounters[i]].groupId;
        counter[0] = m_MyCounters[m_EnabledCounters[i]].counterId;
        glSelectPerfMonitorCountersAMD(m_monitor, GL_FALSE, group, 1, &counter[0]);
    }
    glDeletePerfMonitorsAMD(1, &m_monitor);
}

void QCOMCounters::BeginSample(uint32_t eventId) {
    FUN_INFO("BeginSample, eventId:%u", eventId);
    m_EventId = eventId;
    glBeginPerfMonitorAMD(m_monitor);
}

void QCOMCounters::EndSample() {
    glEndPerfMonitorAMD(m_monitor);
    ReadData();
    /* for(uint32_t counterId : m_EnabledCounters)
    {
      const CounterDescription &desc = m_CounterDescriptions[counterId - 1];
      CounterValue data;
      data.u64 = 0;
      if(desc.resultType == CompType::UInt)
      {
        data.u64 = m_Api->ReadCounterInt(m_Ctx, counterId);
      }
      else if(desc.resultType == CompType::Float)
      {
        data.d = m_Api->ReadCounterDouble(m_Ctx, counterId);
      }
      m_CounterData[m_EventId][counterId] = data;
    }*/
}

uint32_t QCOMCounters::GetCounterId(uint32_t groupId, uint32_t counterId) {
    uint32_t i = 0;
    for (auto &c : m_MyCounters) {
        if (c.groupId == groupId && c.counterId == counterId) {
            return i;
        }
        i++;
    }
    return -1;
}

std::string to_string_2(double x) {
    char a[20];
    sprintf(a, "%.2f", x);
    return a;
}

std::string hr(uint64_t val) { // human readable
    if (val / 1000000000 != 0) {
        auto ret = double(val) / 1000000000;
        return to_string_2(ret) + " G";
    } else if (val / 1000000 != 0) {
        auto ret = double(val) / 1000000;
        return to_string_2(ret) + " M";
    } else if (val / 1000 != 0) {
        auto ret = double(val) / 1000;
        return to_string_2(ret) + " M";
    } else {
        return to_string(val);
    }
}

void QCOMCounters::ReadData() {
    auto enabledCount =
        std::count_if(m_EnabledCountersSupported.begin(), m_EnabledCountersSupported.end(), [](bool v) { return v; });
    FUN_INFO("total enable: %d", enabledCount);

    std::vector<GPUCounter> cs;
    for (int i = 0; i < m_EnabledCounters.size(); i++) {
        if (m_EnabledCountersSupported[i]) {
            auto index = m_EnabledCounters[i];
            auto c = (GPUCounter)(index + (int)GPUCounter::FirstQCOM);
            cs.push_back(c);
        }
    }
    GLuint *counterData = nullptr;
    GLuint dataAvail = 0;
    while (!dataAvail) {
        glGetPerfMonitorCounterDataAMD(
            m_monitor, eGL_PERFMON_RESULT_AVAILABLE_AMD, sizeof(GLuint), &dataAvail, nullptr);
    }

    // read the counters
    GLuint resultSize = 0;
    glGetPerfMonitorCounterDataAMD(m_monitor, eGL_PERFMON_RESULT_SIZE_AMD, sizeof(GLint), &resultSize, NULL);

    FUN_INFO("resultSize = %u", resultSize);
    counterData = (GLuint *)malloc(resultSize);

    GLsizei bytesWritten;
    glGetPerfMonitorCounterDataAMD(m_monitor, eGL_PERFMON_RESULT_AMD, resultSize, counterData, &bytesWritten);

    FUN_INFO("bytesWritten = %d", bytesWritten);
    // display or log counter info
    GLsizei wordCount = 0;
    int tmpI = 0;

    while ((4 * wordCount) < bytesWritten) {
        GLuint groupId = counterData[wordCount];
        GLuint counterId = counterData[wordCount + 1];

        // Determine the counter type
        GLuint counterType;
        glGetPerfMonitorCounterInfoAMD(groupId, counterId, eGL_COUNTER_TYPE_AMD, &counterType);
        int internalCounterId = GetCounterId(groupId, counterId);

        if (counterType == GL_UNSIGNED_INT64_AMD) {
            uint64_t counterResult = *(uint64_t *)(&counterData[wordCount + 2]);

            // Print counter result
            wordCount += 4;
            CounterValue data;
            data.u64 = counterResult;
            m_CounterData[m_EventId][internalCounterId] = data;
            //            if (m_EnabledCountersSupported[internalCounterId]) {
            auto desc = GetCounterDescription(cs[tmpI]);
            FUN_INFO(
                "group:%u, counter:%u, counterResult[%d] = %s  (64bit) %s:%s",
                groupId,
                counterId,
                tmpI,
                hr(counterResult).c_str(),
                desc.category.c_str(),
                desc.name.c_str());
            //            }
        } else if (counterType == GL_FLOAT) {
            float counterResult = *(float *)(&counterData[wordCount + 2]);

            // Print counter result
            wordCount += 3;
            //            if (m_EnabledCountersSupported[internalCounterId]) {
            FUN_INFO("counterResult[%d] = %f   (float)", tmpI, counterResult);
            //            }
        } else {
            FUN_INFO("unkonw counter type %04x", counterType);
        }
        tmpI++;
    }

    if (counterData != nullptr) {
        free(counterData);
    }
}

rdcarray<GPUCounter> QCOMCounters::GetPublicCounterIds() { return m_CounterIds; }

CounterDescription QCOMCounters::GetCounterDescription(GPUCounter index) {
    //  FUN_INFO("get counter desc, index:%d, internal index:%d", (int)index, (int)index - (int)GPUCounter::FirstQCOM -
    //  1);
    return m_CounterDescriptions[(int)index - (int)GPUCounter::FirstQCOM - 1];
}

void QCOMCounters::EnableCounter(GPUCounter counter) {
    FUN_INFO("Enable counter:%u", (uint32_t)counter);
    uint32_t id = (uint32_t)counter - (uint32_t)GPUCounter::FirstQCOM;
    m_EnabledCounters.push_back(id);
}

void QCOMCounters::DisableAllCounters() { m_EnabledCounters.clear(); }

rdcarray<CounterResult>
QCOMCounters::GetCounterData(const rdcarray<uint32_t> &eventIDs, const rdcarray<GPUCounter> &counters1) {
    rdcarray<GPUCounter> counters;
    if (counters1.empty()) {
        for (auto c : m_EnabledCounters) {
            counters.emplace_back((GPUCounter)c);
        }
    } else {
        counters = counters1;
    }

    rdcarray<CounterResult> result;
    for (size_t i = 0; i < eventIDs.size(); i++) {
        uint32_t eventId = eventIDs[i];
        for (size_t j = 0; j < counters.size(); j++) {
            GPUCounter counter = counters[j];
            uint32_t counterId = (uint32_t)counter - (uint32_t)GPUCounter::FirstQCOM;
            const CounterDescription &desc = GetCounterDescription(counter);
            const CounterValue &data = m_CounterData[eventId][counterId];
            if (desc.resultType == CompType::UInt) {
                result.push_back(CounterResult(eventId, counter, data.u64));
            } else if (desc.resultType == CompType::Float) {
                result.push_back(CounterResult(eventId, counter, data.d));
            }
        }
    }
    return result;
}

uint32_t QCOMCounters::GetPassCount() { return 1; }

QCOMCounters::QCOMCounters() : m_EventId(0), m_passIndex(0) {}

QCOMCounters::~QCOMCounters() {}
#endif