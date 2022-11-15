#include <Arduino.h>
// Comms <---
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
File myFile;
char sdname[] = "log.csv";

// Sensor <---
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;
uint32_t value;
char arr[3];

// Power Saving <---
#include <LowPower.h> // https://github.com/rocketscream/Low-Power

// RTC <---
#include "RTClib.h"
RTC_DS3231 rtc;
//  const byte rtcPin = 0; // Interrupt Pin
const byte wakeUpPin = 2; // Interrupt Pin
// #define CLOCK_INTERRUPT_PIN 2

// Misc <---
float t;
float h;
float p;
float a;

void logData();

const byte ledPin = 15;
volatile byte state = LOW;
void interrupt1();

void setup() {//----------------------------------------------------------------------------------
  Wire.begin();        // join i2c bus (address optional for master)
  pinMode(ledPin, OUTPUT);
  
                          digitalWrite(ledPin, HIGH);
                          delay(250);
                          digitalWrite(ledPin, LOW);
                          delay(250);
                          digitalWrite(ledPin, HIGH);
                          delay(250);
                          digitalWrite(ledPin, LOW);


  // RTC setup <---
  rtc.begin();
  // if (! rtc.begin()) {
  //  // digitalWrite(ledPin, HIGH);
  //   while (1);
  // }
  //if (! rtc.isrunning()) {
 // if (rtc.lostPower()) {
    // Serial.println("RTC lost power!");
    // following line sets the RTC to the date & time this sketch was compiled
 //   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
 // }
  //we don't need the 32K Pin, so disable it

                      delay(250);
                      digitalWrite(ledPin, HIGH);
                      delay(250);
                      digitalWrite(ledPin, LOW);
                      
  
  rtc.disable32K();
  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
  rtc.clearAlarm(1);
  //rtc.clearAlarm(2);
  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  rtc.writeSqwPinMode(DS3231_OFF);
  // turn off alarm 2 (in case it isn't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
  // rtc.disableAlarm(1);
  // //rtc.disableAlarm(2);
  rtc.setAlarm1(DateTime(0, 0, 0, 0, 0, 0), DS3231_A1_PerSecond);



  //MicroSD setup <---
  if (! SD.begin(10)) {
    while (1);
  }
  if (! SD.exists(sdname)){
    myFile = SD.open(sdname, FILE_WRITE);
    if (myFile) {
      myFile.print("Date"); myFile.print(";"); myFile.print("Temp_C"); myFile.print(";"); myFile.print("Hum_%"); myFile.print(";"); myFile.print("Pres_hPa"); myFile.print(";"); myFile.println("Alt_m");
      myFile.close(); 
    } else {
      digitalWrite(ledPin, HIGH);
      while (1);
    }
  }

  // Sensor setup <---
  if (!bme.begin(0x76)) {
    while (1);
  }
  
  // Interrupts setup <---
  pinMode(wakeUpPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(wakeUpPin), interrupt1, FALLING); //Attach interrupt

  delay(500);
}//----------------------------------------------------------------------------------

void loop() {//----------------------------------------------------------------------------------
  
  if (state==true){
    logData();
    state=!state;
    digitalWrite(ledPin, state);
    rtc.clearAlarm(1);
  }
  //blink debug
  // digitalWrite(ledPin, state);

  // Rearm alarm <---
  // delay(1);
  // if(rtc.alarmFired(1)) {
  // rtc.clearAlarm(1);
  // };

  // Sleep <---
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  
}//----------------------------------------------------------------------------------

void interrupt1() {
  state = !state;
  digitalWrite(ledPin, state);
}

void logData() {//----------------------------------------------------------------------------------
  // //Debug
  // Wire.beginTransmission(0x76);
  // Wire.write(0xFA);
  // Wire.endTransmission();
  // arr[0] = Wire.requestFrom(0x76, 3);
  // value = arr[0];
  // value <<= 8;
  // value |= arr[1];
  // value <<= 8;
  // value |= arr[2];
  // Serial.print("t: "); Serial.println(value);

  //Read sensor
  t = bme.readTemperature();
  h = bme.readHumidity();
  p = (bme.readPressure() / 100.0F);
  a = bme.readAltitude(SEALEVELPRESSURE_HPA);
  // if (isnan(t) || isnan(h) || isnan(p)) {
  //   Serial.println("Failed to read sensor");
  // }

  //Debug
  // Serial.println("");
  // Serial.print("t: "); Serial.println(t);
  // Serial.print("h: "); Serial.println(h);
  // Serial.print("p: "); Serial.println(p);
  // Serial.print("a: "); Serial.println(a);
  // Serial.flush();

  //Date
  DateTime now = rtc.now();

  // Logging <---
  myFile = SD.open(sdname, FILE_WRITE);
  if (myFile) {
    //Saving
    myFile.print(now.day(), DEC);
    myFile.print('/');
    myFile.print(now.month(), DEC);
    myFile.print('/');
    myFile.print(now.year(), DEC);
    myFile.print(' ');
    //myFile.print(" (");
    //myFile.print(daysOfTheWeek[now.dayOfTheWeek()]);
    //myFile.print(") ");
    myFile.print(now.hour(), DEC);
    myFile.print(':');
    myFile.print(now.minute(), DEC);
    myFile.print(':');
    myFile.print(now.second(), DEC);
    myFile.print(";");
    
    //Sensor
    if (isnan(t) || isnan(h) || isnan(p)) {
      myFile.println("Failed to read sensor;");
    }
    myFile.print(t);
    myFile.print(";");
    myFile.print(h);
    myFile.print(";");
    myFile.print(p);
    myFile.print(";");
    myFile.println(a);
    myFile.close();
  } else {
    // Serial.print("Error opening "); Serial.println(sdname); //If the file didn't open, print an error:
  }
  // Serial.print("Saved to "); Serial.println(sdname);

  // // Rearm alarm <---
  // delay(1);
  // if(rtc.alarmFired(1)) {
  // rtc.clearAlarm(1);
  // };
}//----------------------------------------------------------------------------------