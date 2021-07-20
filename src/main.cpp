/*
 * DS3231 GND <--> GND UNO 
 * DS3231 VCC <--> 3.3 UNO
 * DS3231 SDA <-->  A4 UNO
 * DS3231 SCL <-->  A5 UNO
 * DS3231 SQW <-->  D2 UNO
 */
#include <Arduino.h>
#include <Wire.h>
#include <RtcDS3231.h> // Git:https://github.com/Makuna/Rtc
#include <avr/sleep.h>

// pin(2) => int(0) on the Uno board
#define INTERRUPT_PIN 2
#define INTERRUPT_NUM 0

RtcDS3231<TwoWire> rtc(Wire);

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime &dt)
{
  char datestring[20];
  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Day(), dt.Month(), dt.Year(),
             dt.Hour(), dt.Minute(), dt.Second());
  Serial.print(datestring);
}

// Initialize the RTC - DS3231
void initRTC()
{
  Serial.println("Begin -- initRTC()");
  Serial.print("Compiled: ");

  rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!rtc.IsDateTimeValid())
  {
    if (rtc.LastError() != 0)
    {
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(rtc.LastError());
    }
    else
    {
      Serial.println("RTC lost confidence in the DateTime!");
      rtc.SetDateTime(compiled);
    }
  }

  if (!rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    rtc.SetIsRunning(true);
  }

  RtcDateTime now = rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    rtc.SetDateTime(compiled);
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  rtc.Enable32kHzPin(false);
  // active first alarm
  rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmOne);

  Serial.println("End -- initRTC()");
}

// awake up
void wakeUpNow()
{
  //execute code here after wake-up before returning to the loop() function
  //timers and code using timers(serial.print and more...)will not work here.
  //we don`t really need to execute any special functions here, since we
  //just want the thing to wake up
  Serial.println("Woke up");
  digitalWrite(LED_BUILTIN, HIGH);

  delay(100);
  // return in the loop
}

// sleep down
void sleepDownNow()
{
  Serial.println("Entering Sleep");
  RtcDateTime alarmTime = rtc.GetDateTime(); // now
  DS3231AlarmOne alarm(
      alarmTime.Day(),
      alarmTime.Hour() + 1, // now.hour + 1
      alarmTime.Minute(),
      alarmTime.Second(),
      DS3231AlarmOneControl_HoursMinutesSecondsDayOfMonthMatch);
  rtc.SetAlarmOne(alarm);

  // throw away any old alarm state before we ran
  rtc.LatchAlarmsTriggeredFlags();

  // select the sleep mode
  // use the power down mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // set the sleep enable bit
  sleep_enable();

  // attach the interrupt on int(0)
  attachInterrupt(INTERRUPT_NUM, wakeUpNow, FALLING);

  // turn off the led to show that the arduino is sleeping
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);

  // activate the sleep mode
  sleep_mode();

  // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP

  //first thing after waking from sleep: disable sleep
  sleep_disable();
  //disables interrupt 0 on pin 2 so the wakeUpNow code will not be executed during normal running
  detachInterrupt(INTERRUPT_NUM);
}

void setup()
{
  Serial.begin(9600);
  // define the interrupt pin as a pullup
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  initRTC();
}

void loop()
{
  Serial.print("Ear sensor : ");
  printDateTime(rtc.GetDateTime());
  Serial.println();
  // simulates actions that the arduino must do 
  delay(10000);
  // put the arduino into sleep mode
  sleepDownNow();
}
