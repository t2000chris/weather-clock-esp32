#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>
#include "inetweather.h"

// String today_date_str = "";
String local_name = "Tuen Mun";
// int local_temperature = 0;
// int local_weather_icon = 0;
// int local_humidity = 0;

int today_max_temp = 0;
int today_min_temp = 0;

String url_base = String("https://data.weather.gov.hk/weatherAPI/opendata/weather.php?dataType=");
String url_today_weather = url_base + "rhrread&lang=en";
String url_forecast = url_base + "fnd&lang=en";
String url_warnings = url_base + "warnsum&lang=en";


// caller need to pass the URL and a pre-created DynamicJsonDocument
int fetch_weather(String url, DynamicJsonDocument* jsonDoc)
{
  HTTPClient http;
  String payload;
  Serial.println("Start getting info from web");
  Serial.println(url);
  
  http.begin(url);

  // start connect 
  int httpCode = http.GET();

  // if we get a reply from server
  if(httpCode > 0){
    // print out the http return code
    // Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // if we successfully get the file from web
    // then we put the sting in JSON objects
    if(httpCode == HTTP_CODE_OK) {
      payload = http.getString();

      DeserializationError error = deserializeJson(*jsonDoc, payload);

      // Check if parsing succeed
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        http.end();
        return 0;
      }
    }
    else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      http.end();
      return 0;
    }
  }
  http.end();
  return 1;
}

bool get_weather_warnings(String warnings[]){
  DynamicJsonDocument doc(7000);
  int fetchWeatherOK = 0;

  fetchWeatherOK = fetch_weather(url_warnings, &doc);
  
  if (fetchWeatherOK){
    // we don't know what keys we'll get here, all keys should have the same name as the warning code
    // therefore we just go through the document root and extract all the keys (warning code)
    JsonObject docRoot = doc.as<JsonObject>();

    int cnt = 0;

    // clear existing warnings
    while(cnt < 4){
      warnings[cnt] = "";
      cnt++;
    }
    cnt = 0;

    // we put all warnings in to the warnings[] although we can only display max 5 warnings
    for (JsonPair keyValue : docRoot) {
      if (cnt == 5){
        break;
      }
      Serial.println("We have new weather warnings.");
      // all keys are in upper case
      String wcode = keyValue.value().getMember("code");
      warnings[cnt] = wcode;     
      cnt++;
    }
    return true;
  }
  else{
    Serial.println("Error fetching weather warnings!!!");
    return false;
  }
}

// return true if get weather ok
bool get_local_weather(Weather *weather, bool &haveNewData){
  // size for 9 days forecast is the largest, and it should be around 6200 bytes
  DynamicJsonDocument doc(7000);
  int fetchWeatherOK = 0;

  fetchWeatherOK = fetch_weather(url_today_weather, &doc);

  if (fetchWeatherOK){
    // loop through to find the area we're interested
    int cnt = 0;
    JsonObject areaObj = doc["temperature"]["data"][cnt];
    JsonObject root = doc.as<JsonObject>();

    // get the update time
    String updateTime = root.getMember("updateTime");

    // only process the weather data if the update time is different than last time
    if(weather->update_time != updateTime){
      haveNewData = true;
      while(!areaObj.isNull()){
        String areaStr = areaObj["place"];
        if ( areaObj["place"] == local_name){

          char charbuf[50];
          local_name.toCharArray(charbuf, 50);
          // Serial.printf("Found %s\n", charbuf);
          
          weather->temperature = areaObj["value"];
          // Serial.printf("Local temperature is %d\n", weather.temperature);

          // store the new update time
          weather->update_time = updateTime;

          break;
        }
        else {
          cnt++;
          areaObj = doc["temperature"]["data"][cnt];
        } 
      }

      // there's only one humidity reading from observatory
      weather->humidity = doc["humidity"]["data"][0]["value"];
      // get the weather icon number (need fix for using the 1st item only?)
      weather->weather_icon = doc["icon"][0];

      // Serial.printf("Local humidity : %d\nLocal weather icon : %d\n", weather.humidity, weather.weather_icon);
    }
    // return true if we can get data from internet
    return true;
  }
  
  // return false if no data
  return false;
}

// get the next 6 days forecast, also get today's forecast for min and max temperature
bool get_forecast_weather(Weather *today, Weather forecastDay[], bool &haveNewData){
  int fetchWeatherOK = 0;
  DynamicJsonDocument doc(7000);
  fetchWeatherOK = fetch_weather(url_forecast, &doc);

  if (fetchWeatherOK) {
    // get the update time
    JsonObject root = doc.as<JsonObject>();
    String updateTime = root.getMember("updateTime");

    // only process the weather data if the update time is different than last time
    if(forecastDay[0].update_time != updateTime){
      haveNewData = true;
      int dateCntOffset = 0;

      String firstDayStr = doc["weatherForecast"][0]["forecastDate"];

      // if the forecast for the first day is today, then we know the min max temperature
      if (firstDayStr == today->date){
        today->min_temp = doc["weatherForecast"][0]["forecastMintemp"]["value"];
        today->max_temp = doc["weatherForecast"][0]["forecastMaxtemp"]["value"];

        // Serial.printf("Today max temperature : %d\nToday min temperature : %d\n", today.max_temp, today.min_temp );
        // start from the 2nd object
        dateCntOffset = 1;
      }
      else {
        Serial.println("Can't find today's forecast from the web.");
      }

      // we only do 6 day forecast
      for(int x=0; x<6; x++){

        // get all weather information
        String fc_date = doc["weatherForecast"][dateCntOffset]["forecastDate"];

        // store all weather information
        forecastDay[x].date = fc_date;
        forecastDay[x].min_temp = doc["weatherForecast"][dateCntOffset]["forecastMintemp"]["value"];
        forecastDay[x].max_temp = doc["weatherForecast"][dateCntOffset]["forecastMaxtemp"]["value"];
        forecastDay[x].weather_icon = doc["weatherForecast"][dateCntOffset]["ForecastIcon"];
        forecastDay[x].update_time = updateTime;

        // char charbuf[50];
        // fc_date.toCharArray(charbuf, 50);
        // Serial.println("------ Weather forecast ------");
        // Serial.printf("Date : %s\nMin Temp : %d\nMax Temp : %d\nIcon Num : %d\n\n",
        //               charbuf, fc_min_temp, fc_max_temp, fc_icon_num);

        dateCntOffset++;
      }
    }
    return true;
  }
  return false;

  
}