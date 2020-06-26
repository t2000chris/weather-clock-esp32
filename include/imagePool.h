#ifndef IMAGEPOOL_H_   /* Include guard */
#define IMAGEPOOL_H_
#include <Adafruit_GFX.h>
#include "images/smallWeather.h"
#include "images/bigWeather.h"

// // define an array for all image numbers
extern const int weatherIconList[];
extern const unsigned char *smallWeatherImages[];
extern const unsigned char *bigWeatherImages[];


int findImageIndex(int weather_icon_number);

#endif
