#pragma once
#include <Arduino.h>

using Callback = void(*)();

using TaskParams = struct tskp { time_t tStart; time_t tStop; time_t tInterval; uint32_t intervalMultiplier;
                                 time_t tCyclePeriod; uint32_t nbrOfCycles;
                                 TaskHandle_t tskHandle; Callback callback;
                                } ;

class StartStopTimer
{
    public:
        StartStopTimer(){}

        void init(Callback cb, uint32_t stackDepth=1000, UBaseType_t tskPriority=1);
        void setCycleStart(time_t tsecStart);
        void setCycleStop(time_t tsecStop);
        void setCycleStartStop(const char startDateTime[], const char stopDateTime[], const char tskInterval[]); 
        void setTaskInterval(time_t tsecInterval);
        void setCyclePeriod(time_t tsecCyclePeriod);
        void setNbrOfCycles(uint32_t nbrOfCycles);
        void setIntervalMultiplier(uint32_t factor);
        void resume();
        void suspend();
        void deleteTask();
        TaskHandle_t getTaskHandle();

    private:
        TaskParams     _tskParams = { 0, 0, 1, 1000, 86400, 1, nullptr, nullptr };
        UBaseType_t    _tskPriority;
        uint32_t       _stackDepth;
        static void    _taskFunction(void *params);
};
