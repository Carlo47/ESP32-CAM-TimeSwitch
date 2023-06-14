## Motivation
Experimenting with the ESP32-CAM module, I soon had the idea to save photos to the SD card at certain time intervals. But this time-lapse function should also be able to be activated during a certain period. It should be possible, for example, to save a photo every 15 minutes during a week from 6 o'clock in the morning until 19 o'clock in the evening. This functionality should also be usable for other tasks and should have a simple interface to the user. So it was obvious to implement this timer function in an own class and to realize it as an independent task under FreeRTOS.

## Operating principle
When the camera is switched on, the first thing to do is to get the current time. The easiest way to do this is via a WiFi connection to the router and a time query at an NTP server. Of course the time should correspond to the current time zone. Now the RTC (real time clock) of the ESP32 is set and the WiFi connection is terminated to save power, if the ESP32 is operated with a battery.

> The diagram below shows the timing diagram. 
>
> The task T is performed repeatedly with a time interval of *taskInterval* seconds.
How often the task is executed in one cycle is determined by the
start and stop time. A complete cycle is repeated *nbrOfCycles*
times, each time at intervals of *cyclePeriod* seconds.

```
           cyclePeriod              cyclePeriod
    /------------------------/------------------------/-----
       cycle 1                  cycle 2
    /----------/             /----------/             /-----
    start      stop
    :          :             :          :             :
____T__T__T__T_______________T__T__T__T_______________T__T__T_
    /--/ 
    taskInterval      
```

> All times are in seconds, including start and stop, which are
specified as unix timestamps. However, the user can specify start
and stop as a string in the format "YYYY-MM-DDDD hh:mm". The
conversion is done by the convenience function setCycleStartStop(),
which sets the cycle period to 1 day (86400 sec) and calculates the
number of cycles to run.


## Time settings
```
Start and stop time on same day.
Interval: 00:05
Start: 2023-06-05 09:15
Stop : 2023-06-08 12:30
```
These settings mean: 
- Take a photo every 5 minutes, beginning at
09:15 and ending at 12:30 during 4 days.
- TaskInterval is 5 minutes
- CyclePeriod is 1 day
- nbrOfcycles is 4

```
Start and stop time on different days (crossing midnight).
Interval: 00:30
Start: 2023-06-05 21:30
Stop : 2023-06-06 07:00
```
These settings mean: 
- Take a photo every half an hour, beginning
at 21:30 in the evening and ending in the morning of the next day at 06:00.
- TaskInterval is 30 minutes
- CyclePeriod is 1 day
- nbrOfCycles is 1 


## Features to be realized
- Get the time from NTP server and set the RTC of the ESP32
- Set time for start and stop (unix timestamp)
- Set the task interval in which the task will be executed
- Set the cycle period in which the cycle will be repeated
- Set the number of cycles to be performed
- Set the user defined action
- Suspend the task
- Resume the task
- Stop the task


## Example Program
The program shows how the library can be used to easily
execute multiple time-limited tasks in parallel, e.g.

- Flash the builtin white flash led several times
- Blink the builtin red led during a given period
- Output date and time continously every 5 seconds to the monitor
- Take a photo every 5 minutes during several hours


