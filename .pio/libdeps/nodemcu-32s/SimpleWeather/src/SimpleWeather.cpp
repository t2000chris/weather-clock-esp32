#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "SimpleWeather.h"

#define DEBUG

#ifndef DEBUG_PRINT
  #ifdef DEBUG
    #define DEBUG_PRINT(x)  Serial.println (x)
  #else
    #define DEBUG_PRINT(x)
  #endif
#endif

//const char fingerprint[] PROGMEM = "6E 33 1E DC B8 84 7C D7 40 B2 2C D2 80 F2 4A 38 1B D1 1D CE 6E FA 63 B9 91 FB 4A E9 58 FC 46 37"; // Darksky fingerprint

OpenWeather::OpenWeather(String Key, String City){
  //_Key = Key;
  //_City = City;
  _forecast = false;
  _url = "/data/2.5/weather?q=" + City + "&appid=" + Key +"&units=metric";
}

OpenWeather::OpenWeather(String Key, float lat, float longi){
  //_Key = Key;
  //_City = City;
  _forecast = false;
  _url = "/data/2.5/weather?lat=" + String(lat) + "&long=" + String(longi) + "&appid=" + Key +"&units=metric";
}

OpenWeather::OpenWeather(String Key, String City, boolean forecast){
  //_Key = Key;
  //_City = City;
  _forecast = forecast;
  if(!forecast)
    _url = "/data/2.5/weather?q=" + City + "&appid=" + Key +"&units=metric&cnt=1";
  else
    _url = "/data/2.5/forecast?q=" + City + "&appid=" + Key +"&units=metric&cnt=2";
}

void OpenWeather::updateStatus(weatherData *w){
  //httpsClient.setFingerprint(fingerprint);
  const char *openweather = "api.openweathermap.org";
  const int httpsPort = 443;  //HTTPS= 443 and HTTP = 80
  WiFiClientSecure httpsClient;
  const size_t capacity = 2*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 6*JSON_OBJECT_SIZE(1) + 3*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 5*JSON_OBJECT_SIZE(8) + 570;
  httpsClient.setInsecure();
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  //Serial.print("HTTPS Connecting");
  int r=0; //retry counter
  while((!httpsClient.connect(openweather, httpsPort)) && (r < 30)){
      delay(100);
      Serial.print(".");
      r++;
  }

  DEBUG_PRINT("requesting URL: ");
  DEBUG_PRINT(openweather+_url);

  httpsClient.print(String("GET ") + _url + " HTTP/1.1\r\n" +
               "Host: " + openweather + "\r\n" +
               "Connection: close\r\n\r\n");

  DEBUG_PRINT("request sent");

  while (httpsClient.connected()) {
    _Response = httpsClient.readStringUntil('\n');
    if (_Response == "\r") {
      DEBUG_PRINT("headers received");
      DEBUG_PRINT(_Response);
      break;
    }
  }

  DEBUG_PRINT("reply was:");
  DEBUG_PRINT("==========");
  //httpsClient.readStringUntil('\n'); // The API sends an extra line with just a number. This breaks the JSON parsing, hence an extra read
  while(httpsClient.connected()){
    _Response = httpsClient.readString();
    DEBUG_PRINT(_Response); //Print response
  }
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc,_Response);
  if(!_forecast){
    w->description = doc["weather"][0]["description"].as<String>();
    w->weather = doc["weather"][0]["main"].as<String>();
    w->id = doc["weather"][0]["id"].as<int>();
    w->current_Temp = doc["main"]["temp"].as<float>();
    w->min_temp = doc["main"]["temp_min"].as<float>();
    w->max_temp = doc["main"]["temp_max"].as<float>();
    w->humidity = doc["main"]["temp_max"].as<float>();
    if(w->id <700) w->rain = doc["rain"]["1h"].as<float>();
    else w->rain = 0;
  } else
  {
    // Currently set to get forecast 3 hours from now
    w->description = doc["list"][1]["weather"][0]["description"].as<String>(); 
    w->weather = doc["list"][1]["weather"][0]["main"].as<String>();
    w->id = doc["list"][1]["weather"]["id"].as<int>();
    w->current_Temp = doc["list"][1]["main"]["temp"].as<float>();
    w->min_temp = doc["list"][1]["main"]["temp_min"].as<float>();
    w->max_temp = doc["list"][1]["main"]["temp_max"].as<float>();
    w->humidity = doc["list"][1]["main"]["temp_max"].as<float>();
    if(w->id <700) w->rain = doc["list"][1]["rain"]["3h"].as<float>();
    else w->rain = 0;
  }
}

String OpenWeather::getResponse(){
  return _Response;
}

// DarkSky API

void Darksky::updateURL(String Key, float lat, float longi, unsigned long timestamp){
  _url = "/forecast/" + Key +"/" + String(lat) + "," + String(longi) +"," + String(timestamp) + "?exclude=minutely,hourly,daily,alerts,flags&units=si";
}

void Darksky::updateURL(String Key, float lat, float longi){
  _url = "/forecast/" + Key +"/" + String(lat) + "," + String(longi) +"?exclude=minutely,hourly,daily,alerts,flags&units=si";
}

Darksky::Darksky(String Key, float lat, float longi){
  _url = "/forecast/" + Key +"/" + String(lat) + "," + String(longi) +"?exclude=minutely,hourly,daily,alerts,flags&units=si";
}

Darksky::Darksky(String Key, float lat, float longi, unsigned long timestamp){
  _url = "/forecast/" + Key +"/" + String(lat) + "," + String(longi) +"," + String(timestamp) + "?exclude=minutely,hourly,daily,alerts,flags&units=si";
}


void Darksky::updateStatus(weatherData *w){
  const char *darksky = "api.darksky.net";
  const int httpsPort = 443;  //HTTPS= 443 and HTTP = 80
  WiFiClientSecure httpsClient;
  //httpsClient.setFingerprint(fingerprint);
  httpsClient.setInsecure();
  httpsClient.setTimeout(15000); // 15 Seconds
  //delay(1000);

  //Serial.print("HTTPS Connecting");
  int r=0; //retry counter
  while((!httpsClient.connect(darksky, httpsPort)) && (r < 30)){
      delay(100);
      DEBUG_PRINT(".");
      r++;
  }

  DEBUG_PRINT("requesting URL: ");
  DEBUG_PRINT(darksky+_url);
  DEBUG_PRINT(String("GET ") + _url + " HTTP/1.1\r\n" +
               "Host: " + darksky + "\r\n" +
               "Connection: close\r\n\r\n");

  httpsClient.print(String("GET ") + _url + " HTTP/1.1\r\n" +
               "Host: " + darksky + "\r\n" +
               "Connection: close\r\n\r\n");

  DEBUG_PRINT("request sent");

  while (httpsClient.connected()) {
    _Response = httpsClient.readStringUntil('\n');
    if (_Response == "\r") {
      DEBUG_PRINT("headers received");
      DEBUG_PRINT(_Response);
      break;
    }
  }

  DEBUG_PRINT("reply was:");
  DEBUG_PRINT("==========");
  //httpsClient.readStringUntil('\n'); // The API sends an extra line with just a number. This breaks the JSON parsing, hence an extra read
  while(httpsClient.connected()){
    _Response = httpsClient.readString();
    DEBUG_PRINT(_Response); //Print response
  }
  const size_t capacity = JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(19) + 350;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc,_Response);

    w->description = doc["currently"]["summary"].as<String>();
    w->weather = doc["currently"]["icon"].as<String>();
    w->id = 0; //doc["currently"].as<int>();
    w->current_Temp = doc["currently"]["apparentTemperature"].as<float>(); //Use Apparent Temperature
    w->min_temp = 0; //doc["currently"].as<float>();
    w->max_temp = 0; //doc["currently"].as<float>();
    w->humidity = doc["currently"]["humidity"].as<float>();
    w->rain = doc["currently"]["precipProbability"].as<float>();

}

String Darksky::getResponse(){
  return _Response;
}
