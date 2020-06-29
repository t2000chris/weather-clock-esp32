#include "imagePool.h"
#include "images/warnings.h"

extern const int weatherIconList[] = {50,51,52,53,54,
                                 60,61,62,63,64,65,
                                 70,71,72,73,74,75,76,77,
                                 80,81,82,83,84,85,
                                 90,91,92,93};

extern const String warningIconList[] = {
            "WCOLD", "WFIRER", "WFIREY", "WFNTSA", "WFROST", "WHOT", "WL", "WMSGNLTC1", "WMSGNLTC3",
            "WMSGNLTC8NE", "WMSGNLTC8NW", "WMSGNLTC8SE", "WMSGNLTC8SW", "WMSGNLTC9", "WMSGNLTC10", 
            "WRAINA", "WRAINB", "WRAINR", "WTCSGNL", "WTMW", "WTS"
};

extern const unsigned char* smallWeatherImages[] = {
            bm_smallWeather1, bm_smallWeather2, bm_smallWeather3, bm_smallWeather4, bm_smallWeather5,
            bm_smallWeather6, bm_smallWeather7, bm_smallWeather8, bm_smallWeather9, bm_smallWeather10,
            bm_smallWeather11, bm_smallWeather12, bm_smallWeather13, bm_smallWeather14, bm_smallWeather15, 
            bm_smallWeather16, bm_smallWeather17, bm_smallWeather18, bm_smallWeather19, bm_smallWeather20, 
            bm_smallWeather21, bm_smallWeather22, bm_smallWeather23, bm_smallWeather24, bm_smallWeather25, 
            bm_smallWeather26, bm_smallWeather27, bm_smallWeather28, bm_smallWeather29};


extern const unsigned char* bigWeatherImages[] = {
            bm_bigWeather1, bm_bigWeather2, bm_bigWeather3, bm_bigWeather4, bm_bigWeather5,
            bm_bigWeather6, bm_bigWeather7, bm_bigWeather8, bm_bigWeather9, bm_bigWeather10,
            bm_bigWeather11, bm_bigWeather12, bm_bigWeather13, bm_bigWeather14, bm_bigWeather15, 
            bm_bigWeather16, bm_bigWeather17, bm_bigWeather18, bm_bigWeather19, bm_bigWeather20, 
            bm_bigWeather21, bm_bigWeather22, bm_bigWeather23, bm_bigWeather24, bm_bigWeather25, 
            bm_bigWeather26, bm_bigWeather27, bm_bigWeather28, bm_bigWeather29};


extern const unsigned char* warnWeatherImages[] = {
            bm_warnings1, bm_warnings2, bm_warnings3, bm_warnings4, bm_warnings5,
            bm_warnings6, bm_warnings7, bm_warnings8, bm_warnings9, bm_warnings10,
            bm_warnings11, bm_warnings12, bm_warnings13, bm_warnings14, bm_warnings15, 
            bm_warnings16, bm_warnings17, bm_warnings18, bm_warnings19, bm_warnings20, 
            bm_warnings21
};


int findImageIndex(int weather_icon_number){
    int imgIndex = -1;
    for(int n=0; n < 29; n++){
      if(weatherIconList[n] == weather_icon_number){
        imgIndex = n;
        return imgIndex;
      }
    }
    return imgIndex;
}

int findImageIndex(String warningCode){
  int imgIndex = -1;
  for(int n=0; n < 21; n++){
      if(warningIconList[n] == warningCode){
        imgIndex = n;
        return imgIndex;
      }
    }
    return imgIndex;
}
 
