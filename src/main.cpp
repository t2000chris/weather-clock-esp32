// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

#include "images/system.h"
#include "imagePool.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#include <GxEPD2_BW.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <TimeAlarms.h>

WiFiUDP ntpUDP;

WiFiMulti wifiMulti;

// for RTC 
#include <Wire.h> 
#include <RtcDS3231.h>
RtcDS3231<TwoWire> Rtc(Wire);

// ------- include all fonts -------------
#include "fonts/monofonto10pt7b.h"
#include "fonts/monofonto18pt7b.h"
#include "fonts/monofonto21pt7b.h"
#include "fonts/monofonto26pt7b.h"
#include "fonts/monofonto70pt7b.h"
#include "fonts/monofonto80pt7b.h"

#define CLOCK_FONT monofonto80pt7b
#define WEATHER_FONT monofonto70pt7b
#define DATE_FONT monofonto26pt7b
#define TEMPERATURE_FONT monofonto21pt7b
#define DATE_FONT_S monofonto18pt7b
#define ERROR_FONT monofonto10pt7b

#include "inetweather.h"

// define the pins for the e-ink display
// This is for nodemcu-32s pinout
// GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

// This is for Waveshare ESP32 eink driver board
// There's some special handling (pin remap for this board)
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=5*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));
/**  This is the Wareshare ESP32 driver board eink pin out
#define EPD_SCK_PIN  13
#define EPD_MOSI_PIN 14
#define EPD_CS_PIN   15
#define EPD_RST_PIN  26
#define EPD_DC_PIN   27
#define EPD_BUSY_PIN 25
**/

// Digital pin connected to the DHT sensor
// #define DHTPIN 14
#define DHTPIN 32
#define DHTTYPE DHT22
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

#define ONBOARD_LED  2

// WIFI SSID and password
const char* ssid       = "t2000home-2.4G";
const char* password   = "alicekatrina";

// NTP server and set our GMT+8 timezone
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;
const int   daylightOffset_sec = 3600;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, 60000);

// Use the chinese days below once I found a good font
// String weekday_str[7] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
String weekday_str[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int indoor_humidity = 0;
int indoor_temperature = 0;

// we can only show a maximum of 5 warnings in our e-ink display
String weather_wanings[5];

struct Weather local_weather_today;
struct Weather forecast[6];

// all bool for status checking
bool have_wifi = false;
bool have_ntp = false;
bool have_rtc = false;
bool have_rtc_battery = false;
bool have_temperature_sensor = false;
bool have_local_weather = false;
bool have_fcast_weather = false;
bool have_warn_weather = false;

// Connect WIFI
void connectWifi()
{
  Serial.printf("Connecting to %s ", ssid);
  wifiMulti.addAP(ssid, password);
  if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("WiFi not connected");
      have_wifi = false;
  }
  else
  {
    Serial.println(" CONNECTED");
    have_wifi = true;
  }
}

// this is to create a "today" string to search the forecast from the weather info
String getTodayDateString(){
  // Construct a date string like "20191125" for searching in JSON 
  if (Rtc.IsDateTimeValid()){
    RtcDateTime timeNow = Rtc.GetDateTime();
    char timeStringBuff[50]; //50 chars should be enough
    snprintf(timeStringBuff, sizeof(timeStringBuff), "%u%02u%02u", timeNow.Year(), timeNow.Month(), timeNow.Day());
    String dateNow(timeStringBuff);
    // Serial.println("Today's Date is ");
    // Serial.println(dateNow);
    have_rtc_battery = true;
    return dateNow;
  }
  else
  {
    // if return false means the battery is dead or the date and time has never been set
    have_rtc_battery = false;
    return "";
  }
} 

// get NTP time, return true if success
bool setTimeWithNTP()
{
  if(timeClient.update()) {
    unsigned long epochTime = timeClient.getEpochTime()-946684800UL;
    Rtc.SetDateTime(epochTime);
    have_ntp = true;
    return true;
  }
  else {
    Serial.println("NTP server sycn failed!!");
    have_ntp = false;
    return false;
  }
}



void getIndoorTemperature(){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  have_temperature_sensor = true;

  // Compute heat index in Celsius (isFahreheit = false)
  // float hic = dht.computeHeatIndex(t, h, false);

  indoor_humidity = h;
  indoor_temperature = t;

  // Serial.print(F("Humidity: "));
  // Serial.print(h);
  // Serial.print(F("%  Temperature: "));
  // Serial.print(t);
  // Serial.print(F("°C  Heat index: "));
  // Serial.print(hic);
  // Serial.println(F("°C "));
}


//****************** All UI draw from here ***********************//

// draw all layout lines and static images for e-ink
void drawStaticUI(){
  display.fillScreen(GxEPD_WHITE);
  display.drawFastHLine(0, 230, 400, GxEPD_BLACK);
  display.drawFastHLine(0, 336, 800, GxEPD_BLACK);

  display.drawFastVLine(250, 230, 106, GxEPD_BLACK);
  display.drawFastVLine(400, 0, 230, GxEPD_BLACK);
  int box_width = 800/6;
  
  for(int x=1; x<7; x++){
    int my_width = box_width*x;
    // Serial.println(my_width);
    display.drawFastVLine(my_width, 336, 144, GxEPD_BLACK);
  }

  // draw all static images
  display.drawBitmap(20, 238, bm_systemHouse, 96, 91, GxEPD_BLACK);
  display.drawBitmap(132, 238, bm_systemThermometer, 19, 42, GxEPD_BLACK);
  display.drawBitmap(122, 288, bm_systemHumidity, 42, 42, GxEPD_BLACK);
  
  display.drawBitmap(655, 188, bm_arrowUp, 42, 42, GxEPD_BLACK);
  display.drawBitmap(655, 238, bm_arrowDown, 42, 42, GxEPD_BLACK);
  display.drawBitmap(655, 288, bm_systemHumidity, 42, 42, GxEPD_BLACK);

  display.drawBitmap(750, 25, bm_celsiusBig, 35, 35, GxEPD_BLACK);
  display.drawBitmap(218, 236, bm_celsiusSmall, 23, 23, GxEPD_BLACK);
  display.drawBitmap(218, 290, bm_percentage, 23, 23, GxEPD_BLACK);

  display.drawBitmap(760, 185, bm_celsiusSmall, 23, 23, GxEPD_BLACK);
  display.drawBitmap(760, 235, bm_celsiusSmall, 23, 23, GxEPD_BLACK);
  display.drawBitmap(760, 290, bm_percentage, 23, 23, GxEPD_BLACK);
}

void drawDate(){
  RtcDateTime timeNow = Rtc.GetDateTime();
  char timeStringBuff[50];
  // just put c_str() here to change from String to char* to avoid complie warning
  snprintf(timeStringBuff, sizeof(timeStringBuff), "%u/%u/%u %s", timeNow.Day(), timeNow.Month(), timeNow.Year(), weekday_str[timeNow.DayOfWeek()].c_str());
  String dateNow_str(timeStringBuff);
  display.setFont(&DATE_FONT); 
  display.setCursor(25,50);
  display.print(dateNow_str);

}

void drawClock(){
  // if RTC has a valid time
  if (Rtc.IsDateTimeValid()){
    // draw the clock
    RtcDateTime timeNow = Rtc.GetDateTime();
    char timeStringBuff[50]; //50 chars should be enough
    snprintf(timeStringBuff, sizeof(timeStringBuff), "%02u:%02u", timeNow.Hour(), timeNow.Minute());
    String timeNow_str(timeStringBuff);
    display.setFont(&CLOCK_FONT);
    display.setCursor(5,200);
    display.print(timeNow_str);
  }
}

  
void drawIndoorTemperature(){
  // draw temperature
  char tempStringBuff[20];
  snprintf(tempStringBuff, sizeof(tempStringBuff), "%d", indoor_temperature);
  String tempNow_str(tempStringBuff);
  display.setFont(&TEMPERATURE_FONT);
  display.setCursor(175, 270);
  display.print(tempNow_str);

  // draw humidity
  snprintf(tempStringBuff, sizeof(tempStringBuff), "%d", indoor_humidity);
  String humidNow_str(indoor_humidity);
  display.setCursor(175, 323);
  display.print(humidNow_str);
}

  
  // u8g2Fonts.drawGlyph(0, 10, 0x0e200);  // Power Supply
  // u8g2Fonts.drawGlyph(12, 10, 0x0e201);  // Charging
  // u8g2Fonts.drawGlyph(24, 10, 0x0e10a);  // Right Arrow

void drawWeatherNow(){
  // draw the weather image
  int imgIndex;
  imgIndex = findImageIndex(local_weather_today.weather_icon);
  display.drawBitmap(410, 10, bigWeatherImages[imgIndex], 216, 216, GxEPD_BLACK);
  // draw temperature now
  display.setFont(&WEATHER_FONT);
  display.setCursor(630,155);
  display.print(String(local_weather_today.temperature));

  // draw today's highest and lowest temperature
  // after 12noon, we won't be able to get today's hightest and lowest temp
  // if so then we just print out "no info"
  if (local_weather_today.min_temp != 0){
    // we have the temperature info, just print that out
    display.setFont(&TEMPERATURE_FONT); 
    display.setCursor(710,222);
    display.print(String(local_weather_today.max_temp));
    display.setCursor(710,272);
    display.print(String(local_weather_today.min_temp));
  }
  else{
    // otherwise we just print out "no info"
    display.setFont(&ERROR_FONT);
    display.setCursor(710,218);
    display.print("N/A");
    display.setCursor(710,267);
    display.print("N/A");
  }
  
  // draw today's humidity
  display.setFont(&TEMPERATURE_FONT);
  display.setCursor(710,322);
  display.print(String(local_weather_today.humidity));
}

void drawForecast(){
  int xoffset_name = 134;
  int xoffset_temp = 134;
  int i;
  int realx_name;
  int realx_temp;
  int weekdaynum;

  // get the time now
  RtcDateTime timeNow = Rtc.GetDateTime();
  weekdaynum = timeNow.DayOfWeek();

  // loop through each day and draw forcast 
  // total 6 days
  for (i=0; i<6; i++){
    // display all Weekday names
    display.setFont(&DATE_FONT_S); 
    realx_name = 33 + (i*xoffset_name);
    display.setCursor(realx_name,367);
    // always display the next day
    weekdaynum = weekdaynum + 1;
    if (weekdaynum > 6){
      weekdaynum = 0;
    }
    display.print(weekday_str[weekdaynum]);

    // draw all forcast icons
    int imgIndex;
    imgIndex = findImageIndex(forecast[i].weather_icon);
    display.drawBitmap(realx_name, 375, smallWeatherImages[imgIndex], 66, 66, GxEPD_BLACK);


    // draw forcast temperatures
    display.setFont(&DATE_FONT_S); 
    realx_temp = 18 + (i*xoffset_temp);
    display.setCursor(realx_temp,473);
    char tempStringBuff[50];
    snprintf(tempStringBuff, sizeof(tempStringBuff), "%d-%d", forecast[i].min_temp, forecast[i].max_temp);
    String temp_str(tempStringBuff);
    // forecast[x].weather_icon;
    display.print(temp_str);
  }
}

// we can draw max of 5 warnings
void drawWeatherWarnings(){
  int cnt = 0;
  int imgIndex;
  int xoffset = 93;
  int x;

  // in the current UI design, can only display 4 warings max
  while(weather_wanings[cnt] != NULL && cnt < 4){
    // Serial.println("got warnings");
    // Serial.println(weather_wanings[cnt]);

    x = 260 + (xoffset*cnt);
    imgIndex = findImageIndex(weather_wanings[cnt]);
    display.drawBitmap(x, 238, warnWeatherImages[imgIndex], 90, 90, GxEPD_BLACK);
    cnt++;
  }
}

void drawErrorMsg(){
  int cnt = 0;
  int y_offset = 17;
  display.setFont(&ERROR_FONT);

  // if we don't have wifi
  if(!have_wifi){
    int y = 250 + (y_offset * cnt);
    display.setCursor(260 ,y);
    display.println("WiFi not connected!!");
    cnt++;
  }

  if(!have_rtc){
    int y = 250 + (y_offset * cnt);
    display.setCursor(260 ,y);
    display.println("Real Time Clock not found!!");
    cnt++;
  }

  if(!have_rtc_battery){
    int y = 250 + (y_offset * cnt);
    display.setCursor(260 ,y);
    display.println("Real Time Clock battery is dead or the time is never set!!");
    cnt++;
  }

  if(!have_temperature_sensor){
    int y = 250 + (y_offset * cnt);
    display.setCursor(260 ,y);
    display.println("Temperature sensor not found!!");
    cnt++;
  }

  if(!have_ntp){
    int y = 250 + (y_offset * cnt);
    display.setCursor(260 ,y);
    display.println("NTP not connected!!");
    cnt++;
  }

  if(!have_local_weather){
    int y = 250 + (y_offset * cnt);
    display.setCursor(260 ,y);
    display.println("Cannot get local weather!!");
    cnt++;
  }

  if(!have_fcast_weather){
    int y = 250 + (y_offset * cnt);
    display.setCursor(260 ,y);
    display.println("Cannot get forecast weather!!");
    cnt++;
  }
  
}


void einkSetup(){
  Serial.println("elinkSetup ");
  // do not need to rotate the screen
  display.setRotation(0);
  // full window mode is the initial mode, set it anyway
  display.setFullWindow();
  // display.setFont(&FreeMonoBold9pt7b);   
             
  display.setTextColor(GxEPD_BLACK);
}



// --------------------- UI Ends --------------------- //

void i2cScanner(){
  Serial.println("Scanning I2C Addresses Channel");
  uint8_t cnt=0;
  for(uint8_t i=0;i<128;i++){
    Wire.beginTransmission(i);
    uint8_t ec=Wire.endTransmission(true);
    if(ec==0){
      if(i<16)Serial.print('0');
      Serial.print(i,HEX);
      cnt++;
    }
    else Serial.print("..");
    Serial.print(' ');
    if ((i&0x0f)==0x0f)Serial.println();
    }
  Serial.print("Scan Completed, ");
  Serial.print(cnt);
  Serial.println(" I2C Devices found.");
  Serial.println("---------------------------------------");

}



void runEverySecond(){
  // if( digitalRead(ONBOARD_LED) ){
  //   digitalWrite(ONBOARD_LED, LOW);
  //   Serial.println("LED set to LOW");
  // }
  // else{
  //   digitalWrite(ONBOARD_LED, HIGH);
  //   Serial.println("LED set to HIGH");
  // }

  RtcDateTime timeNow = Rtc.GetDateTime();
  if (timeNow.Second() == 0){
    // Serial.println("1 min, redraw the clock");

    // update the clock
    display.setPartialWindow(5, 80, 390, 130);
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      drawClock();
    } while (display.nextPage());

    // update indoor temperature
    display.setPartialWindow(173, 235, 42, 90);
    do
    {
      display.fillScreen(GxEPD_WHITE);
      drawIndoorTemperature();
    } while (display.nextPage());
  }
}

// we change the date at 0:00
// FIXME - need to redraw everything since date changed
void runEveryMidnite(){
  // display.setPartialWindow(5, 1, 390, 60);

  setTimeWithNTP();


  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    drawDate();
  } while (display.nextPage());
}



void setup() {
  Serial.begin(115200);
  // // this eink display init has to run in the very beginning
  // // otherwise it won't work
  display.init(115200);
  // start the temperature sensor and Real Time Clock

  // setup onboard LED
  pinMode(ONBOARD_LED, OUTPUT);

  dht.begin();
  Rtc.Begin();

  // connect wifi first
  connectWifi();

  // run the function every second.  I wanna use this lib instead of playing with hardware clock in esp32
  Alarm.timerRepeat(1, runEverySecond);
  // change date every 00:00
  Alarm.alarmRepeat(0,0,0,runEveryMidnite);

  // Alarm.timerRepeat(20, updateIndoorTemperature);

  // do all internet tasks if we have wifi.
  if (have_wifi) {
    // check if we got RTC, if so we do the NTP time sync
    if(Rtc.GetIsRunning()){
      have_rtc = true;
      setTimeWithNTP();
    }
    else{
      Serial.println("RTC module is not running or not found!!");
    }

    // if NTP time is ok, that means we have internet (since we don't do any ping test)
    if (have_ntp){
      local_weather_today.date = getTodayDateString();

      // get local and all forcast weather info
      have_local_weather = get_local_weather(&local_weather_today);
      have_fcast_weather = get_forecast_weather(&local_weather_today, forecast);
      have_warn_weather = get_weather_warnings(weather_wanings);

      // Serial.println("------ Weather today ------");
      //   Serial.printf("Date : %s\nTemp : %d\nHumidity : %d\nIcon Num : %d\n\n",
      //               local_weather_today.date, local_weather_today.temperature, local_weather_today.humidity, local_weather_today.weather_icon);

      // Serial.println("------ Weather forecast ------");
      // for(int x=0; x<6; x++){
      //   Serial.printf("Date : %s\nMin Temp : %d\nMax Temp : %d\nIcon Num : %d\n\n",
      //               forecast[x].date, forecast[x].min_temp, forecast[x].max_temp, forecast[x].weather_icon);
      // }
    }
  }


  // these task are not internet related
  // get the indoor temperature from sensor
  getIndoorTemperature();

  // start to draw UI 
  einkSetup();

  // *** special handling for Waveshare ESP32 Driver board *** //
  // ********************************************************* //
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(13, 12, 14, 15); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
  // *** end of special handling for Waveshare ESP32 Driver board *** //
  // **************************************************************** //

  display.firstPage();
  do
  {
    drawStaticUI();
    drawDate();
    drawClock();
    drawIndoorTemperature();
    if(have_ntp){
      drawForecast();
      drawWeatherNow();
      drawWeatherWarnings();
    }
    drawErrorMsg();
  } while (display.nextPage());

  // delay(10000);
  // display.println("Clear screen");
  // display.clearScreen();
  // delay(5000);
  // display.powerOff();

}

void loop() {
  // Wait a few seconds between measurements.

  // getIndoorTemperature();

  // RtcDateTime currTime = Rtc.GetDateTime();
  // Serial.println("RTC time---");
  // printDateTime(currTime);
  // Serial.println("NTP time---");
  // Serial.println(timeClient.getFormattedTime());

  // if (!RTC_check()){
  //   Serial.println("RTC not running!!");
  // }
  // delay(10000);
  // display.firstPage();
  // do
  // {
  //   drawClock();
  // } while (display.nextPage());
  // for (int i=0; i< 29; i++){
  //       Serial.println("image bitmap is");
  //       Serial.println((long int)imagePoolSmall[i].bitmap);
  //   }
  Alarm.delay(500);
}