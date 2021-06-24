#ifndef INETWEATHER_H_   /* Include guard */
#define INETWEATHER_H_

// #include <Arduino.h>
extern WiFiMulti wifiMulti;

struct Weather {
    String date;
    int temperature;
    int max_temp;
    int min_temp;
    int weather_icon;
    int humidity;
    String update_time;
};

int fetch_weather(String url, DynamicJsonDocument* jsonDoc);
bool get_local_weather(Weather *weather, bool &haveNewData);
bool get_forecast_weather(Weather *today, Weather forcastDay[], bool &haveNewData);
bool get_weather_warnings(String warnings[]);

#endif
