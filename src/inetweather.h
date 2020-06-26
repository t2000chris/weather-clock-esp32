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
};

int fetch_weather(String url, DynamicJsonDocument* jsonDoc);
void get_local_weather(Weather *weather);
void get_forecast_weather(Weather today, Weather forcastDay[]);

#endif
