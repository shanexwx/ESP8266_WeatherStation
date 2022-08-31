
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>  
#include <WiFiClientSecureBearSSL.h>  
#include "HeFeng.h"


HeFeng::HeFeng() {

}
/*
String url = "https://devapi.qweather.com/v7/weather/now?location=" + location + "&key=" + key + "&gzip=n";
        String tmp = root【"now"】【"temp"】;
        String fl = root【"now"】【"feelsLike"】;
        String hum = root【"now"】【"humidity"】;
        String wind_sc = root【"now"】【"windScale"】;
        String cond_code = root【"now"】【"icon"】;
        String cond_txt = root【"now"】【"pressure"】;
        
String url = "https://devapi.qweather.com/v7/weather/3d?location=" + location + "&key=" + key + "&gzip=n";
          String tmp_min = root【"daily"】【i】【"tempMin"】;
         String tmp_max = root【"daily"】【i】【"tempMax"】;
          String datestr = root【"daily"】【i】【"fxDate"】;
          String cond_code = root【"daily"】【i】【"iconDay"】;
*/
 
void HeFeng::doUpdateCurr(HeFengCurrentData *data, String key,String location) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);    
    client->setInsecure();    
    HTTPClient https;  
    String url = "https://devapi.qweather.com/v7/weather/now?location=" + location + "&key=" + key + "&gzip=n";
    Serial.print("[HTTPS] begin...\n");  
    if (https.begin(*client, url)) {  // HTTPS    
      // start connection and send HTTP header  
      int httpCode = https.GET();    
      // httpCode will be negative on error  
      if (httpCode > 0) {  
        // HTTP header has been send and Server response header has been handled  
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);    
     
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  
          String payload = https.getString();  
          Serial.println(payload);  
          DynamicJsonDocument  jsonBuffer(2048);
          deserializeJson(jsonBuffer, payload);
          JsonObject root = jsonBuffer.as<JsonObject>();
          
         String tmp = root["now"]["temp"];   
         data->tmp=tmp;             
         String fl=root["now"]["feelsLike"];         
         data->fl=fl;
         String hum=root["now"]["humidity"];         
         data->hum=hum;
         String wind_sc=root["now"]["windScale"];         
         data->wind_sc=wind_sc;
         String cond_code=root["now"]["icon"];  
         String meteoconIcon=getMeteoconIcon(cond_code);
         String cond_txt=root["now"]["pressure"];
         data->cond_txt=cond_txt;
         data->iconMeteoCon1=meteoconIcon;
         //iconMeteoCon=meteoconIcon String meteoconIcon=getMeteoconIcon(cond_code);
         //iconMeteCon
         
        }  
      } else {  
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
         data->tmp="-1";                 
         data->fl="-1";       
         data->hum="-1";      
         data->wind_sc="-1";         
         data->cond_txt="no network";
         data->iconMeteoCon1=")";
      }  
  
      https.end();  
    } else {  
      Serial.printf("[HTTPS] Unable to connect\n");
         data->tmp="-1";                 
         data->fl="-1";       
         data->hum="-1";      
         data->wind_sc="-1";         
         data->cond_txt="no network";
         data->iconMeteoCon1=")";
    }  

}

void HeFeng::doUpdateFore(HeFengForeData *data, String key,String location) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);    
    client->setInsecure();    
    HTTPClient https;  
    String url = "https://devapi.qweather.com/v7/weather/3d?location=" + location + "&key=" + key + "&gzip=n";
    Serial.print("[HTTPS] begin...\n");  
    if (https.begin(*client, url)) {  // HTTPS    
      // start connection and send HTTP header  
      int httpCode = https.GET();    
      // httpCode will be negative on error  
      if (httpCode > 0) {  
        // HTTP header has been send and Server response header has been handled  
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);    
     
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  
          String payload = https.getString();  
          Serial.println(payload);  
          DynamicJsonDocument  jsonBuffer(8192);
          deserializeJson(jsonBuffer, payload);
          JsonObject root = jsonBuffer.as<JsonObject>();
        int i;
           for (i=0; i<3; i++){
         String tmp_min=root["daily"][i]["tempMin"];         
         data[i].tmp_min=tmp_min;             
         String tmp_max=root["daily"][i]["tempMax"];         
         data[i].tmp_max=tmp_max;  
          String datestr=root["daily"][i]["fxDate"];         
         data[i].datestr=datestr.substring(5,datestr.length()); 
         String cond_code=root["daily"][i]["iconDay"];  
         String meteoconIcon=getMeteoconIcon(cond_code);        
         data[i].iconMeteoCon=meteoconIcon;
           }
        }  
      } else {  
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());  
          int i;
           for (i=0; i<3; i++){               
         data[i].tmp_min="-1";          
         data[i].tmp_max="-1";    
         data[i].datestr="N/A";
         data[i].iconMeteoCon=")";
           }
      }  
  
      https.end();  
    } else {  
      Serial.printf("[HTTPS] Unable to connect\n");  
        int i;
           for (i=0; i<3; i++){               
         data[i].tmp_min="-1";          
         data[i].tmp_max="-1";    
         data[i].datestr="N/A";
         data[i].iconMeteoCon=")";
           }
    }  

}

   String HeFeng::getMeteoconIcon(String cond_code){
    if(cond_code=="100"||cond_code=="9006"){return "B";}
    if(cond_code=="999"){return ")";}
    if(cond_code=="104"){return "D";}
     if(cond_code=="500"){return "E";}
      if(cond_code=="503"||cond_code=="504"||cond_code=="507"||cond_code=="508"){return "F";}
       if(cond_code=="499"||cond_code=="901"){return "G";}
        if(cond_code=="103"){return "H";}
         if(cond_code=="502"||cond_code=="511"||cond_code=="512"||cond_code=="513"){return "L";}
          if(cond_code=="501"||cond_code=="509"||cond_code=="510"||cond_code=="514"||cond_code=="515"){return "M";}
           if(cond_code=="102"){return "N";}
            if(cond_code=="213"){return "O";}
               if(cond_code=="302"||cond_code=="303"){return "P";}
                  if(cond_code=="305"||cond_code=="308"||cond_code=="309"||cond_code=="314"||cond_code=="399"){return "Q";}
                         if(cond_code=="306"||cond_code=="307"||cond_code=="310"||cond_code=="311"||cond_code=="312"||cond_code=="315"||cond_code=="316"||cond_code=="317"||cond_code=="318"){return "R";}
                         if(cond_code=="200"||cond_code=="201"||cond_code=="202"||cond_code=="203"||cond_code=="204"||cond_code=="205"||cond_code=="206"||cond_code=="207"||cond_code=="208"||cond_code=="209"||cond_code=="210"||cond_code=="211"||cond_code=="212"){return "S";}
                             if(cond_code=="300"||cond_code=="301"){return "T";}
                                 if(cond_code=="400"||cond_code=="408"){return "U";}
                           if(cond_code=="407"){return "V";}
                                if(cond_code=="401"||cond_code=="402"||cond_code=="403"||cond_code=="409"||cond_code=="410"){return "W";}
                                     if(cond_code=="304"||cond_code=="313"||cond_code=="404"||cond_code=="405"||cond_code=="406"){return "X";}
                              if(cond_code=="101"){return "Y";}
    return ")";   
    }    
 
