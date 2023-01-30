/**
 * @file main.cpp
 * @author Tobias Poppe
 * @brief
 * @version 0.1
 * @date 2022-04-14
 *
 * @copyright Copyright (c) 2022
 *
 * Based on http://www.netzmafia.de/skripten/hardware/Arduino/Sleep/index.html
 * Enables the NBIOT-Board and sensors, wait for the NBIOT-Feedback-Signal fall, go to deepsleep for 5 minutes
 */

#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define LED_PIN (13)
#define MOSFET_PIN (10)
#define FEEDBACK_PIN (8)

volatile int sleep_cycles = 0;

uint32_t RunTime;
uint32_t StartTime;
uint16_t wdt_wake_up_time = 0;
uint16_t temp_wdt_time = 0;
int wakeup_every_minutes = 5;

ISR(WDT_vect)
/* Watchdog Interrupt Service Routine */
{
}

// Enter deepsleep
void enter_sleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  wdt_wake_up_time = wdt_wake_up_time + (millis() - temp_wdt_time);
  power_all_disable(); // disable all unused peripherals
  sleep_mode();        // Start sleeping
  sleep_disable();     // Wake-up entry point
  power_timer0_enable();
  temp_wdt_time = millis();
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOSFET_PIN, OUTPUT);
  pinMode(FEEDBACK_PIN, INPUT);

  /* Setup watchdog timers */
  MCUSR &= ~(1 << WDRF);              // WDT reset flag
  WDTCSR |= (1 << WDCE) | (1 << WDE); // set WDCE
  WDTCSR = 1 << WDP0 | 1 << WDP3;     // Prescaler 8s
  WDTCSR |= 1 << WDIE;                // free WDT Interrupt flag

  // set all unused pins to output and low to avoid floating pins
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW);
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  pinMode(A1, OUTPUT);
  digitalWrite(A1, LOW);
  pinMode(A2, OUTPUT);
  digitalWrite(A2, LOW);
  pinMode(A3, OUTPUT);
  digitalWrite(A3, LOW);
  pinMode(A4, OUTPUT);
  digitalWrite(A4, LOW);
  pinMode(A5, OUTPUT);
  digitalWrite(A5, LOW);
  pinMode(A6, OUTPUT);
  digitalWrite(A6, LOW);
  pinMode(A7, OUTPUT);
  digitalWrite(A7, LOW);
  delay(100);
}

void loop()
{
  if (!sleep_cycles)
  {
    uint32_t temp;
    StartTime = millis();
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(MOSFET_PIN, HIGH);
    delay(1000); // give the NBIOT-Board 500ms to come up
    while (digitalRead(FEEDBACK_PIN) && (millis() - StartTime) <= 120000)
      ; // wait for the NBIOT-Board to finish the tasks, Timeout 2 minutes
    digitalWrite(MOSFET_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    if (RunTime < millis())
    {
      RunTime = (millis() - StartTime);
    }
    else
    {
      RunTime = 10;
    }
    temp = (((wakeup_every_minutes * 60000UL) - RunTime - wdt_wake_up_time) / 8000UL);
    sleep_cycles = temp;
    temp = ((wakeup_every_minutes * 60000UL) - ((sleep_cycles)*8000UL) - RunTime - wdt_wake_up_time);
    wdt_wake_up_time = 0;
    delay(temp);
    enter_sleep();
  }
  else
  {
    sleep_cycles = sleep_cycles - 1;
    enter_sleep();
  }
}