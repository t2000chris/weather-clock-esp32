/*
  SimpleWeather.cpp - Library to get weather from Openweather
  Created by Dushyant Ahuja, September 2019.
  Released into the public domain.
*/

#ifndef SimpleWeather_h
#define SimpleWeather_h

#include "Arduino.h"
#include <ESP8266WiFi.h>


struct weatherData{
  String weather;
  String description;
  int id;
  float current_Temp;
  float min_temp;
  float max_temp;
  float humidity;
  float rain;
};

class OpenWeather
{
  public:
    OpenWeather(String Key, String City); 
    OpenWeather(String Key, float lat, float longi);
    OpenWeather(String Key, String City, boolean forecast);
    void updateStatus(weatherData *w);
    String getResponse();
  private:
    String _Response;
    String _url;
    boolean _forecast;
};

class Darksky
{
  public:
    Darksky(String Key, float lat, float longi);
    Darksky(String Key, float lat, float longi, unsigned long timestamp);
    void updateStatus(weatherData *w);
    void updateURL(String Key, float lat, float longi, unsigned long timestamp);
    void updateURL(String Key, float lat, float longi);
    String getResponse();
  private:
    String _Response;
    String _url;
};

#endif
