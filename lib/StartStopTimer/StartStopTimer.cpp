#include "StartStopTimer.hpp"

void StartStopTimer::init(Callback cb, uint32_t stackDepth, UBaseType_t tskPriority)
{
    TaskParams *p = &_tskParams;
    _tskPriority = tskPriority;
    _stackDepth = stackDepth;
    _tskParams.callback = cb;

    BaseType_t res = xTaskCreate
    (
        _taskFunction,          // Function to be called by the task
        "Task",                 // Name of the task (for debugging)
        stackDepth,             // Stack size (bytes)
        static_cast<void *>(&_tskParams),  // Parameter to be passed to the taskFunction
        tskPriority,            // Task priority
        &_tskParams.tskHandle   // Task handle (used to delete/suspend/resume task)
    );
    if (res != pdPASS)
    {
        log_e("!!! task not created, initialization stopped !!!");
        while (true) { delay(10); }
    }

    //log_i("task creation result = %d stack: %d, priority: %d\n", res, _stackDepth, _tskPriority);
    
    vTaskSuspend(_tskParams.tskHandle);

    //log_i("start: %ld, stop: %ld, interval: %ld\n", p->tStart, p->tStop, p->tInterval);
    log_i("==> done %p", _tskParams.tskHandle);
}

/**
 * Convenience function to convert the date time strings
 * into the needed timestamps and the task interval into
 * seconds. The cycle period ist set to one day (86400 sec)
 * and the number of cycles (days) to be performed is computed.
 * Example:
 *      startDateTime  (CEST): "2023-06-13 22:40" --> 1686688800
 *      stopDateTime   (CEST): "2023-06-14 06:15" --> 1686716100
 *      taskInterval:          "00:05"            --> 300
*/
void StartStopTimer::setCycleStartStop(const char startDateTime[], const char stopDateTime[], const char taskInterval[]) 
{
    char buf[20];
    tm td = {0};
    int tskHH;
    int tskMM;
    
    sscanf(taskInterval, "%d:%d", &tskHH, &tskMM);
    _tskParams.tInterval = 60 * (60 * tskHH + tskMM);
    
    sscanf(startDateTime, "%d-%d-%d %d:%d", &td.tm_year, &td.tm_mon, &td.tm_mday, &td.tm_hour, &td.tm_min);
    td.tm_year -= 1900;
    td.tm_mon -= 1;
    td.tm_isdst = -1;
    _tskParams.tStart = mktime(&td);
    strftime(buf, sizeof(buf), "%F %H:%M", &td);
    log_i("%s", buf);

    sscanf(stopDateTime, "%d-%d-%d %d:%d", &td.tm_year, &td.tm_mon, &td.tm_mday, &td.tm_hour, &td.tm_min);
    td.tm_year -= 1900;
    td.tm_mon -= 1;
    td.tm_isdst = -1;
    _tskParams.tStop = mktime(&td);
    strftime(buf, sizeof(buf), "%F %H:%M", &td);
    log_i("%s", buf);

    _tskParams.tCyclePeriod = 86400;
    _tskParams.nbrOfCycles = 1 + (_tskParams.tStop - _tskParams.tStart) / _tskParams.tCyclePeriod;
    log_i("start: %d, stop: %d, diff: %d", _tskParams.tStart, _tskParams.tStop, _tskParams.tStop - _tskParams.tStart);
    log_i("taskInterval: %d", _tskParams.tInterval);
    log_i("nbrOfCycles: %d", _tskParams.nbrOfCycles); 
};

void StartStopTimer::setCycleStart(time_t tsecStart)  { _tskParams.tStart = tsecStart; }

void StartStopTimer::setCycleStop(time_t tsecStop)    { _tskParams.tStop = tsecStop; }

void StartStopTimer::setTaskInterval(time_t tsecInterval)  { _tskParams.tInterval = tsecInterval; }

void StartStopTimer::setCyclePeriod(time_t tsecPeriod)      { _tskParams.tCyclePeriod = tsecPeriod; }

void StartStopTimer::setNbrOfCycles(uint32_t nbrOfCycles)   { _tskParams.nbrOfCycles = nbrOfCycles; }

void StartStopTimer::setIntervalMultiplier(uint32_t factor) { _tskParams.intervalMultiplier = factor; }

void StartStopTimer::resume()  { vTaskResume(_tskParams.tskHandle); }

void StartStopTimer::suspend() { vTaskSuspend(_tskParams.tskHandle); }

void StartStopTimer::deleteTask() { vTaskDelete(_tskParams.tskHandle); _tskParams.tskHandle = nullptr; }

TaskHandle_t StartStopTimer::getTaskHandle() { return _tskParams.tskHandle; }

void StartStopTimer::_taskFunction(void *params)
{
    TaskParams *p = static_cast<TaskParams *>(params);
    
    //log_i("nbrOfCycles=%d", p->nbrOfCycles);

    for (uint32_t n = 0; n < p->nbrOfCycles; n++)
    {
        //log_i("cycle: %d", n);
        // Wait until start time of 1st cycle is reached
        while (p->tStart > time(nullptr)) { vTaskDelay(pdMS_TO_TICKS(10)); } 
        p->tStart = time(nullptr);  // remember start time of cycle

        // Do task until stop time is reached
        while (time(nullptr) < p->tStop)
        {
            p->callback(); // call the function supplied by the user
            //log_i("wait interval: %d * %d", p->intervalMultiplier, p->tInterval);
            vTaskDelay(pdMS_TO_TICKS(p->intervalMultiplier * p->tInterval));
        }
        p->tStart += p->tCyclePeriod;
        p->tStop  += p->tCyclePeriod;        
    }

    vTaskDelete(p->tskHandle); p->tskHandle = nullptr; // delete task
    //vTaskSuspend(p->tskHandle); // suspend the task until resume is called by the user
};


