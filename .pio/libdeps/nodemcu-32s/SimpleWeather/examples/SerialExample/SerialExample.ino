#include <ESP8266WiFi.h>
#include "SimpleWeather.h"


//************************************
// Replace with your wifi credentials
const char * ssid = "**************";
const char * password = "*************";

// Get and API Key by registering on
// https://openweathermap.org/api
// and https://darksky.net/dev/account


String Key = "*************";
String DKey = "*************";


weatherData w;
OpenWeather weather(Key, "London,uk");
OpenWeather forecast(Key, "London,uk",1);

Darksky ds(DKey,51.5074,0.1278,1572170400);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("WiFi Failed");
      while(1) {
          delay(1000);
      }
  }
}

void loop() {
  Serial.println("\nOpenWeather Current:");
  weather.updateStatus(&w);
  Serial.print("Weather: ");
  Serial.println(w.weather);
  Serial.print("Description: ");
  Serial.println(w.description);
  Serial.print("ID: ");
  Serial.println(w.id);
  Serial.print("Current Temp: ");
  Serial.println(w.current_Temp);
  Serial.print("Min Temp: ");
  Serial.println(w.min_temp);
  Serial.print("Max Temp: ");
  Serial.println(w.max_temp);
  Serial.print("Humidity: ");
  Serial.println(w.humidity);
  Serial.print("Rain: ");
  Serial.println(w.rain);
  Serial.print("Full Response: ");
  Serial.println(weather.getResponse().c_str());

  Serial.println("\nOpenWeather Forecast:");
  forecast.updateStatus(&w);
  Serial.print("Weather: ");
  Serial.println(w.weather);
  Serial.print("Description: ");
  Serial.println(w.description);
  Serial.print("ID: ");
  Serial.println(w.id);
  Serial.print("Current Temp: ");
  Serial.println(w.current_Temp);
  Serial.print("Min Temp: ");
  Serial.println(w.min_temp);
  Serial.print("Max Temp: ");
  Serial.println(w.max_temp);
  Serial.print("Humidity: ");
  Serial.println(w.humidity);
  Serial.print("Rain: ");
  Serial.println(w.rain);
  Serial.print("Full Response: ");
  Serial.println(forecast.getResponse().c_str());

  Serial.println("\nDarksky Current:");
  ds.updateStatus(&w);
  Serial.print("Weather: ");
  Serial.println(w.weather);
  Serial.print("Description: ");
  Serial.println(w.description);
  Serial.print("ID: ");
  Serial.println(w.id);
  Serial.print("Current Temp: ");
  Serial.println(w.current_Temp);
  Serial.print("Min Temp: ");
  Serial.println(w.min_temp);
  Serial.print("Max Temp: ");
  Serial.println(w.max_temp);
  Serial.print("Humidity: ");
  Serial.println(w.humidity);
  Serial.print("Rain: ");
  Serial.println(w.rain);
  Serial.print("Full Response: ");
  Serial.println(ds.getResponse().c_str());

  delay(600000);       // Wait for 600 seconds
  // put your main code here, to run repeatedly:
}