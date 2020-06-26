// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

// #include "images/bigWeather.h"
// #include "images/smallWeather.h"
// #include "images/warnings.h"
#include "images/system.h"
#include "imagePool.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Arduino.h>
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
// This is for nodemcu-32s my own pinout
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
// This is for Waveshare ESP32 eink driver board
// GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=5*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));

// Digital pin connected to the DHT sensor
#define DHTPIN 14
#define DHTTYPE DHT22
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// WIFI SSID and password
const char* ssid       = "t2000home-2.4G";
const char* password   = "alicekatrina";

// NTP server and set our GMT+8 timezone
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;
const int   daylightOffset_sec = 3600;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, 60000);

// String weekday_str[7] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
String weekday_str[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// String today_date_str = "";

int indoor_humidity = 0;
int indoor_temperature = 0;

struct Weather local_weather_today;
struct Weather forecast[6];

bool have_wifi = false;
bool have_ntp = false;
bool have_weather = false;

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

// just to display the local time
// void printLocalTime()
// {
  // struct tm timeinfo;
  // if(!getLocalTime(&timeinfo)){
  //   Serial.println("Failed to obtain time");
  //   // Serial.println("Try to connect NTP server again");
  //   // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //   // return;
  // }
  
  // time_t tnow = now();
  // char charbuf[50];
  // snprintf(charbuf, sizeof(charbuf), "%d/%d/%d", day(tnow), month(tnow), year(tnow));

  // char timeStringBuff[50]; //50 chars should be enough
  // // Construct a date string like "20191125" for searching in JSON 
  // strftime(timeStringBuff, sizeof(timeStringBuff), "%Y%m%d", &timeinfo);
  // String mystr(timeStringBuff);
  // // today_date_str = mystr;

  // local_weather_today.date = mystr;

  // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  // Serial.print("Next day is : ");
  // timeinfo.tm_mday += 1;
  // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  // int test = timeinfo.tm_wday;

  // just convert it to char* to avoid warning when compile
  // char charbuf[50];
  // chi_days[test].toCharArray(charbuf, 50);
  // Serial.printf("Today's weekday number is : %s\n", charbuf);
// }

String getTodayDateString(){
  // Construct a date string like "20191125" for searching in JSON 
  if (Rtc.IsDateTimeValid()){
    RtcDateTime timeNow = Rtc.GetDateTime();
    char timeStringBuff[50]; //50 chars should be enough
    snprintf(timeStringBuff, sizeof(timeStringBuff), "%u%u%u", timeNow.Year(), timeNow.Month(), timeNow.Day());
    String dateNow(timeStringBuff);
    // Serial.println("Today's Date is ");
    // Serial.println(dateNow);
    return dateNow;
  }
  else
  {
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

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

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
  // if RTC has a valid time
  if (Rtc.IsDateTimeValid()){
    RtcDateTime timeNow = Rtc.GetDateTime();
    char timeStringBuff[50];
    snprintf(timeStringBuff, sizeof(timeStringBuff), "%u/%u/%u %s", timeNow.Day(), timeNow.Month(), timeNow.Year(), weekday_str[timeNow.DayOfWeek()]);
    String dateNow_str(timeStringBuff);
    display.setFont(&DATE_FONT); 
    display.setCursor(25,50);
    display.print(dateNow_str);
  }
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

void drawErrorMsg(String msg){
  display.setFont(&ERROR_FONT); 
  display.setCursor(260 ,255);
  display.println(msg);
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



// To check if RTC is working, if not then we just update the RTC with the NTP time
bool RTC_check(){
  bool boolCheck = true;
  if (!Rtc.IsDateTimeValid()){
    Serial.println("RTC DateTime is not valid!  Updating DateTime");
    setTimeWithNTP();
    boolCheck = false;
  }

  if(!Rtc.GetIsRunning()){
    Serial.println("RTC was not actively running, starting now.  Updating Date Time");
    Rtc.SetIsRunning(true);
    setTimeWithNTP();
    boolCheck = false;
  }
  return boolCheck;
}


void runEverySecond(){
  if (Rtc.IsDateTimeValid()){
    RtcDateTime timeNow = Rtc.GetDateTime();
    if (timeNow.Second() == 0){
      Serial.println("1 min, redraw the clock");

      display.setPartialWindow(5, 80, 390, 130);
      display.firstPage();
      // display.fillScreen(GxEPD_WHITE);
      // drawClock();
      // display.nextPage();
      do
      {
        display.fillScreen(GxEPD_WHITE);
        drawClock();
      } while (display.nextPage());
    }
  }
}


void setup() {
  Serial.begin(115200);
  // // this eink display init has to run in the very beginning
  // // otherwise it won't work
  display.init(115200);
  // start the temperature sensor and Real Time Clock
  dht.begin();
  Rtc.Begin();

  // connect wifi first
  connectWifi();

  // run the function every second.  I wanna use this lib instead of playing with hardware clock in esp32
  Alarm.timerRepeat(1, runEverySecond); 

  // do all internet tasks if we have wifi.
  if (have_wifi) {
    // init and get the time
    setTimeWithNTP();
    // if NTP time is ok, that means we have internet (since we don't do any ping test)
    if (have_ntp){
      local_weather_today.date = getTodayDateString();

      // get local and all forcast weather info
      get_local_weather(&local_weather_today);
      get_forecast_weather(local_weather_today, forecast);

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
  display.firstPage();
  do
  {
    drawStaticUI();
    drawDate();
    drawClock();
    drawIndoorTemperature();
    if(! have_wifi){
      drawErrorMsg("No WiFi connected!!");
    }
    if(have_ntp){
      drawForecast();
      drawWeatherNow();
    }
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