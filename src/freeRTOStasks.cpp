/**
 * Program      freeRTOStasks.cpp
 * 
 * Author       2023-06-14 Charles Geiser (www.dodeka.ch) 
 * 
 * Purpose      Shows how to use my library StartStopTimer
 *              and how to get the time from an NTP server and set the internal
 *              RTC (real time clock) of the ESP32 and then start 4 independent 
 *              FreeRTOS tasks.
 *                - task1: Blink the red led every second for 10 ms.
 *                - task2: Print date and time every 5 seconds to the monitor
 *                - task3: Flash SOS signals, 3 times 4 signals in a task
 *                - task4: Take a photo every 5 minutes during a set period
 * 
 * Board        ESP32-CAM with builtin red led on GPIO 33 and white flash led on GPIO 4
 * 
 * Remarks      The WiFi connection is only used to set the RTC. After getting the time,  
 *              the connection is closed.
 * 
 * Library      Task creation and time management is hidden from the user in the 
 *              StartStopTimer library class. The user only has to set the start and stop 
 *              time as well as the interval in which the task will be executed periodically. 
 *              The task executes a callback function, which is supplied by the user when 
 *              calling the task initialization. 
 * 
 * Wiring       none
 * 
 * Reference    https://savjee.be/blog/multitasking-esp32-arduino-freertos/    
*/

#include <Arduino.h>
#include <soc/rtc_cntl_reg.h>	// bypass brownout problems (not activated in setup)
#include <WiFi.h>
#include <time.h>
#include "StartStopTimer.hpp"

const int LED_BUILTIN = 33; // GPIO of the red led
const int FLASH_LED   = 4;  // GPIO of the white flash led
const char NTP_SERVER_POOL[] = "ch.pool.ntp.org";
const char TIME_ZONE[]       = "MEZ-1MESZ-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
const char HOST_NAME[]       = "ESP-CAM_TASK";


// WiFi credentials 
const char SSID[]     = "your SSID";
const char PASSWORD[] = "your password";

// Function prototypes
void initLeds();
void initWiFi(const char hostname[], const char ssid[], const char password[]);
void initRTC(const char timezone[], const char ntpserver[]);
void initTask1();
void initTask2();
void initTask3();
void initTask4();

void blinkLed();  // task1 callback
void showTime();  // task2 callback
void flashSOS();  // task3 callback
void takePhoto(); // task4 callback

StartStopTimer task1;
StartStopTimer task2;
StartStopTimer task3;
StartStopTimer task4;


void setup() 
{
  Serial.begin(115200);
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector to prevent restart of the ESP32
  initLeds();
  initWiFi(HOST_NAME, SSID, PASSWORD);
  initRTC(TIME_ZONE, NTP_SERVER_POOL);
  initTask1();
  initTask2();
  initTask3();
  initTask4();
  //log_i("stack 1 %d", uxTaskGetStackHighWaterMark(task1.getTaskHandle()));
  //log_i("stack 2 %d", uxTaskGetStackHighWaterMark(task2.getTaskHandle()));
  //log_i("stack 3 %d", uxTaskGetStackHighWaterMark(task3.getTaskHandle()));
  //log_i("stack 4 %d", uxTaskGetStackHighWaterMark(task4.getTaskHandle()));
  log_i("==> done");
}


void loop() 
{
  vTaskDelay(pdMS_TO_TICKS(1000)); 
}


void initLeds()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn builtin led off

  pinMode(FLASH_LED, OUTPUT);
  digitalWrite(FLASH_LED, LOW);     // turn flash led off
}


/**
 * Establish the WiFi connection with router
*/
void initWiFi(const char hostname[], const char ssid[], const char password[])
{
  Serial.println("Connecting to WiFi...");
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.println("Connection to WiFi failed. Restarting ESP32 in 5 seconds");
    delay(5000);
    ESP.restart();
  }
  Serial.println("...connected");
  log_i("==> done");
}


/**
 * Initialize the ESP32 RTC with local time
 * and close the no longer needed WiFi connection
*/
void initRTC(const char timezone[], const char ntpserver[])
{
  tm rtcTime;
  configTzTime(timezone, ntpserver);
  while(! getLocalTime(&rtcTime))
  {
    Serial.println("...Failed to obtain time");
  }
  log_i("Got time from NTP Server");
  WiFi.disconnect(true); //rtc is set, wifi connection no longer needed
  log_i("==> done");
}


/**
 * Blink the red builtin led every second during 10 minutes
 * The on-time of the led is defined in the taskfunction blinkLed
*/
void initTask1()
{
  task1.setTaskInterval(1);  // blink every second
  task1.setCycleStart(time(nullptr));
  task1.setCycleStop(time(nullptr) + 600); // blink for 10 minutes
  task1.init(blinkLed, 2000); // initialize the task with callback and stack size
  task1.resume();
}


/**
 * Show the time from now on every 2 seconds and ending after 10 seconds
 * Repeat this cycle 3 times every half minute (cycle period is 30 sec)
*/
void initTask2()
{
  task2.setTaskInterval(2); // show time every 2 seconds
  task2.setCycleStart(time(nullptr));
  task2.setCycleStop(time(nullptr) + 10);
  task2.setCyclePeriod(30);
  task2.setNbrOfCycles(3);
  task2.init(showTime, 2000); // initialize the task with callback and stack size
  task2.resume();
}


/**
 * Flash SOS signals with the white LED
 * One task, i.e. a complete SOS signal, takes 5800 ms (see function flashSOS). 
 * If we choose a task interval of 10 sec and set the time between
 * start and stop to 50 sec, there is room for 4 SOS signals in 
 * this time. A task cycle thus comprises 4 SOS signals. With the 
 * cycle period of 120 sec, these 4 signals are repeated after 
 * 2 minutes and this 3 times because nbrOfCycles is set to 3.
 * In total, 12 SOS signals are emitted.
*/
void initTask3()
{
  task3.setTaskInterval(10);  // flash SOS again after 10 sec
  task3.setCycleStart(time(nullptr));
  task3.setCycleStop(time(nullptr) + 50); 
  task3.setCyclePeriod(120);
  task3.setNbrOfCycles(3);
  task3.init(flashSOS, 2000); // initialize the task with callback and stack size
  task3.resume();
}


/**
 * Take a photo every 5 minutes beginning at
 * start date and time and ending at stop date and time.
 * The convenience function setCycleStartStop converts the
 * date time strings into the needed timestamps and the 
 * task interval into seconds. The cycle period ist set
 * to one day (86400 sec) and the number of cycles (days)
 * to be performed is computed.
*/
void initTask4()
{
  task4.setCycleStartStop("2023-06-13 22:40", "2023-06-14 06:15", "00:05"); 
  task4.init(takePhoto, 2000);
  task4.resume(); 
}


void blinkLed()
{
  digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on
  vTaskDelay(pdMS_TO_TICKS(10));   // Pause the task for 10 ms
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off  
}


void showTime()
{
  tm   rtcTime;
  char buf[40];
  int  bufSize = sizeof(buf);

  getLocalTime(&rtcTime);
  strftime(buf, bufSize, "%B %d %Y %T (%A)",  &rtcTime); // January 15 2019 16:33:20 (Tuesday)
  Serial.printf("%s\n", buf);
}


void flashSOS()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(FLASH_LED, HIGH);  // Turn the LED on
    vTaskDelay(pdMS_TO_TICKS(50));  // Pause the task for 50 ms
    digitalWrite(FLASH_LED, LOW);   // Turn the LED off
    vTaskDelay(pdMS_TO_TICKS(450));
  }

  for (int i = 0; i < 3; i++)
  {
    digitalWrite(FLASH_LED, HIGH);  // Turn the LED on
    vTaskDelay(pdMS_TO_TICKS(150)); // Pause the task for 150 ms
    digitalWrite(FLASH_LED, LOW);   // Turn the LED off
    vTaskDelay(pdMS_TO_TICKS(450));
  }

  for (int i = 0; i < 3; i++)
  {
    digitalWrite(FLASH_LED, HIGH);  // Turn the LED on
    vTaskDelay(pdMS_TO_TICKS(50));  // Pause the task for 50 ms
    digitalWrite(FLASH_LED, LOW);   // Turn the LED off
    vTaskDelay(pdMS_TO_TICKS(450));
  } 
}


void takePhoto()
{
  static int cntPhoto = 0;
  Serial.printf("Photo taken: %d\n", ++cntPhoto);
}
