#include <Arduino.h>
#include <ESP8266WiFi.h>
static WiFiClient espClient;
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
#include <AliyunIoTSDK.h>
//#include <IRremoteESP8266.h>
//#include <IRac.h>
//#include <IRutils.h>
#include <Servo.h>

#include "SH1106Wire.h"
#include "OLEDDisplayUi.h"
#include "Wire.h"
#include "HeFeng.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

#define PRODUCT_KEY "hjkhnI9ApEW"
#define DEVICE_NAME "yxBv2i0utWPskieFPtl6"
#define DEVICE_SECRET "e0aa1312c60844076031528881a2dbaa"
#define REGION_ID "cn-shanghai"

//#define ACSEND ac.sendAc();

/*******************************************************************************************************************************************
// WiFi名称和密码写在下边，只支持2.4g频段的WiFi(web配网不需要用这个)
*******************************************************************************************************************************************/
//const char* WIFI_SSID = "Redmi K50";
//const char* WIFI_PWD = "wxY020617";
/******************************************************************************************************************************************
******************************************************************************************************************************************/


/******************************************************************************************************************************************
空调型号在这改，链接里有对照表
******************************************************************************************************************************************/
uint8_t level;
uint8_t setTemp;
int firm = 34; //型号型号对照https://img2.moeblog.vip/images/Zx9t.png

Servo myservo;
//const uint16_t kIrLed = 12;
//IRac ac(kIrLed);

#define TZ              8       
#define DST_MN          0      //夏令时 [(0)关]


// Setup
const int UPDATE_INTERVAL_SECS = 10 * 60; // 10分钟更新一次天气

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;

const String WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
//const String MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/***************************
 * End Settings
 **************************/
 // Initialize the oled display for address 0x3c

 SH1106Wire     display(I2C_DISPLAY_ADDRESS, 4, 5);   // or SSD1306Wire  display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
 OLEDDisplayUi   ui( &display );

HeFengCurrentData currentWeather;
HeFengForeData foreWeather[3];
HeFeng HeFengClient;

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

const char* HEFENG_KEY="4f78dac8c82d4b439a53269b6a2a80af";
const char* HEFENG_LOCATION="101010100";
time_t now;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;
long timeSinceLastCurrUpdate = 0;

String currTemp="-1.0";
//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyForWeatherUpdate();


// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTime, drawCurrentWeather,drawForecast };

int numberOfFrames = 3;

OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;


bool autoConfig()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  //Serial.print("AutoConfig Waiting......");
   int counter = 0;
  for (int i = 0; i < 20; i++)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      //Serial.println("AutoConfig Success");
      //Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      //Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      WiFi.printDiag(Serial);
      return true;
    }
    else
    {
       delay(500);
    //Serial.print(".");
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display(); 
     counter++; 
    }
  }
  //Serial.println("AutoConfig Faild!" );
  return false;
}

ESP8266WebServer server(80);
String HTML_TITLE = "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\"><title>ESP8266_WEB配网</title>";
String HTML_SCRIPT_ONE = "<script type=\"text/javascript\">function wifi(){var ssid = s.value;var password = p.value;var xmlhttp=new XMLHttpRequest();xmlhttp.open(\"GET\",\"/HandleWifi?ssid=\"+ssid+\"&password=\"+password,true);xmlhttp.send();xmlhttp.onload = function(e){alert(this.responseText);}}</script>";
String HTML_SCRIPT_TWO = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
String HTML_HEAD_BODY_BEGIN = "</head><body>欢迎使用ESP8266_WEB配网，请输入wifi信息:";
String HTML_FORM_ONE = "<form>WiFi名称：<input id='s' name='s' type=\"text\" placeholder=\"请输入WiFi名称\"><br>WiFi密码：<input id='p' name='p' type=\"text\" placeholder=\"请输入WiFi密码\"><br><input type=\"button\" value=\"扫描附近可用网络\" onclick=\"window.location.href = '/HandleScanWifi'\"><input type=\"button\" value=\"连接\" onclick=\"wifi()\"></form>";
String HTML_BODY_HTML_END = "</body></html>";

void handleRoot() {
    //Serial.println("root page");
    String str = HTML_TITLE + HTML_SCRIPT_ONE + HTML_SCRIPT_TWO + HTML_HEAD_BODY_BEGIN + HTML_FORM_ONE + HTML_BODY_HTML_END;
    server.send(200, "text/html", str);
}

void HandleScanWifi() {
    //Serial.println("scan start");

    String HTML_FORM_TABLE_BEGIN = "<table><head><tr><th>序号</th><th>名称</th><th>强度</th></tr></head><body>";
    String HTML_FORM_TABLE_END = "</body></table>";
    String HTML_FORM_TABLE_CON = "";
    String HTML_TABLE;
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    //Serial.println("scan done");
    if (n == 0) {
        //Serial.println("no networks found");
        HTML_TABLE = "NO WIFI !!!";
    }
    else {
        //Serial.print(n);
        //Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
            //Serial.print(i + 1);
            //Serial.print(": ");
            //Serial.print(WiFi.SSID(i));
            //Serial.print(" (");
            //Serial.print(WiFi.RSSI(i));
            //Serial.print(")");
            //Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
            //delay(10);
            HTML_FORM_TABLE_CON = HTML_FORM_TABLE_CON + "<tr><td align=\"center\">" + String(i+1) + "</td><td align=\"center\">" + "<a href='#p' onclick='c(this)'>" + WiFi.SSID(i) + "</a>" + "</td><td align=\"center\">" + WiFi.RSSI(i) + "</td></tr>";
        }

        HTML_TABLE = HTML_FORM_TABLE_BEGIN + HTML_FORM_TABLE_CON + HTML_FORM_TABLE_END;
    }
    //Serial.println("");

    String scanstr = HTML_TITLE + HTML_SCRIPT_ONE + HTML_SCRIPT_TWO + HTML_HEAD_BODY_BEGIN + HTML_FORM_ONE + HTML_TABLE + HTML_BODY_HTML_END;

    server.send(200, "text/html", scanstr);
}

void HandleWifi()
{
    String wifis = server.arg("ssid"); //从JavaScript发送的数据中找ssid的值
    String wifip = server.arg("password"); //从JavaScript发送的数据中找password的值
    //Serial.println("received:"+wifis);
    server.send(200, "text/html", "连接中..");
    WiFi.begin(wifis,wifip);
}

void handleNotFound() { 
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void htmlConfig()
{
    WiFi.mode(WIFI_AP_STA);//设置模式为AP+STA
    WiFi.softAP("WIFI_Clock");

    IPAddress myIP = WiFi.softAPIP();
  
    if (MDNS.begin("clock")) {
      //Serial.println("MDNS responder started");
    }
    
    server.on("/", handleRoot);
    server.on("/HandleWifi", HTTP_GET, HandleWifi);
    server.on("/HandleScanWifi", HandleScanWifi);
    server.onNotFound(handleNotFound);//请求失败回调函数
    MDNS.addService("http", "tcp", 80);
    server.begin();//开启服务器
    //Serial.println("HTTP server started");
    int counter = 0;
    while(1)
    {
        server.handleClient();
        MDNS.update();  
         delay(500);
          display.clear();
          display.drawString(64, 5, "WIFI AP:WIFI_Clock");
          display.drawString(64, 20, "192.168.4.1");
           display.drawString(64, 35, "waiting for config wifi.");
          display.drawXbm(46, 50, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
          display.drawXbm(60, 50, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
          display.drawXbm(74, 50, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
          display.display();  
           counter++;
        if (WiFi.status() == WL_CONNECTED)
        {
            //Serial.println("HtmlConfig Success");
            //Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
            //Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
            //Serial.println("HTML连接成功");
            break;
        }
    }
       server.close();  
       WiFi.mode(WIFI_STA);
    
}

void setup(){
  display.init();
  display.clear();
  display.display();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);//改括号里的数可以调节屏幕亮度，数据范围0-255
  //WiFi.mode(WIFI_STA);
  //WiFi.begin(WIFI_SSID, WIFI_PWD);
  int counter = 0;
  bool wifiConfig = autoConfig();
    if(wifiConfig == false){
        htmlConfig();//HTML配网
    }
    /*
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();
    counter++;
  }
    //if(wifiConfig == false){
        //htmlConfig();
    //}*/
/***********************************************************************************************************************
ui设置
***********************************************************************************************************************/
  ui.setTargetFPS(40);
  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);
  ui.setIndicatorPosition(BOTTOM);// 页码标识符位置TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorDirection(LEFT_RIGHT);// Defines where the first frame is located in the bar.
  ui.setFrameAnimation(SLIDE_UP);// 改括号里的参数可以改变翻页的方向，有向左、向右、向上、向下分别对应SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrames(frames, numberOfFrames);
  ui.setTimePerFrame(7500);//改括号里的数可以调节翻页的间隔时间，单位是毫秒，7500即7500ms=7.5秒。
  ui.setOverlays(overlays, numberOfOverlays);
  ui.init();
  configTime(TZ_SEC, DST_SEC, "pool.ntp.org","0.cn.pool.ntp.org","1.cn.pool.ntp.org");
  updateData(&display);
/***********************************************************************************************************************
***********************************************************************************************************************/


/*********************************************************************************************************************** 
//阿里云基本设置
***********************************************************************************************************************/
  AliyunIoTSDK::begin(espClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
//举例说明：LightSwitch是在设备产品中定义的物联网模型的 id，LightSwitchCallback是回调函数，云端检测到LightSwitch属性更改便会触发回调。
  AliyunIoTSDK::bindData("LightSwitch", LightSwitchCallback);
  //AliyunIoTSDK::bindData("AC_Power", AC_PowerCallback);
  //AliyunIoTSDK::bindData("TempSet", TempSetCallback);
  //AliyunIoTSDK::bindData("FanSpeed", FanSpeedCallback);
  //AliyunIoTSDK::bindData("SleepMode", SleepModeCallback);
  //AliyunIoTSDK::bindData("Auto", AutoModeCallback);
  //AliyunIoTSDK::bindData("Cool", CoolingModeCallback);
  //AliyunIoTSDK::bindData("Dry", DryModeCallback);
  //AliyunIoTSDK::bindData("Fan", FanModeCallback);
  //AliyunIoTSDK::bindData("Heat", HeatModeCallback);
    
// 例：发送数据到云平台，LightLuminance是在设备产品中定义的物联网模型的 id
  //AliyunIoTSDK::send("LightLuminance", 100);
/**********************************************************************************************************************  
**********************************************************************************************************************/

/**********************************************************************************************************************
空调设置
**********************************************************************************************************************
  decode_type_t protocol = (decode_type_t)firm;
  ac.next.protocol = protocol; 
  ac.next.model = 1;  //若发现遥控功能不全可尝试修改这个参数，没有对照表
  ac.next.mode = stdAc::opmode_t::kCool;  //开机为制冷模式
  ac.next.celsius = true;  //温度单位设置为摄氏度，False = 华氏度
  ac.next.degrees = 26;  //默认温度26
  ac.next.fanspeed = stdAc::fanspeed_t::kMedium;  //开机风速中速
  ac.next.swingv = stdAc::swingv_t::kOff;  // 不上下扫风
  ac.next.swingh = stdAc::swingh_t::kOff;  // 不左右扫风
  ac.next.light = false;  // Turn off any LED/Lights/Display that we can.
  ac.next.beep = false;  // Turn off any beep from the A/C if we can.
  ac.next.econo = false;  // Turn off any economy modes if we can.
  ac.next.filter = false;  // Turn off any Ion/Mold/Health filters if we can.
  ac.next.turbo = false;  //关闭超强
  ac.next.quiet = false;  //关闭静音
  ac.next.sleep = -1;  //关闭睡眠
  ac.next.clean = false;  //关闭清洁模式
  ac.next.clock = -1;  //关闭设置
  ac.next.power = false;  //初始化时空调关闭
*********************************************************************************************************************
*********************************************************************************************************************/
  pinMode(14,OUTPUT);
  myservo.attach(14,500,2500);
  Serial.begin(115200);
  myservo.write(90);
}

void loop() {
  AliyunIoTSDK::loop();

  if (millis() - timeSinceLastWUpdate > (1000L*UPDATE_INTERVAL_SECS)) {
    setReadyForWeatherUpdate();
    timeSinceLastWUpdate = millis();
  }
  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display);
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }

   

}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->flipScreenVertically();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}


void updateData(OLEDDisplay *display) {
  //display.flipScreenVertically();
  drawProgress(display, 30, "Updating weather...");

for(int i=0;i<5;i++){
  HeFengClient.doUpdateCurr(&currentWeather, HEFENG_KEY, HEFENG_LOCATION);
  if(currentWeather.cond_txt!="no network"){
    break;}
 }
  drawProgress(display, 50, "Updating forecasts...");
  
 for(int i=0;i<5;i++){
  HeFengClient.doUpdateFore(foreWeather, HEFENG_KEY, HEFENG_LOCATION);
    if(foreWeather[0].datestr!="N/A"){
    break;}
 }
 
  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done...!");
  delay(1000);
}



void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->flipScreenVertically();
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];


  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  String date = WDAY_NAMES[timeInfo->tm_wday];
 
  sprintf_P(buff, PSTR("%04d-%02d-%02d, %s"), timeInfo->tm_year + 1900, timeInfo->tm_mon+1, timeInfo->tm_mday, WDAY_NAMES[timeInfo->tm_wday].c_str());
  display->drawString(64 + x, 5 + y, String(buff));
  display->setFont(ArialMT_Plain_24);

  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(64 + x, 22 + y, String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->flipScreenVertically();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, "Pre: "+currentWeather.cond_txt+" hPa"+" | Wind: "+currentWeather.wind_sc);

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = currentWeather.tmp + "°C" ;
  display->drawString(60 + x, 3 + y, temp);
  display->setFont(ArialMT_Plain_10);
  display->drawString(70 + x, 26 + y, currentWeather.fl+"°C | "+currentWeather.hum+"%");
  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 0 + y, currentWeather.iconMeteoCon1);
}


void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->flipScreenVertically();
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 1);
  drawForecastDetails(display, x + 88, y, 2);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  display->flipScreenVertically();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y, foreWeather[dayIndex].datestr);
  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 12, foreWeather[dayIndex].iconMeteoCon);

  String temp=foreWeather[dayIndex].tmp_min+" | "+foreWeather[dayIndex].tmp_max;
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 34, temp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->flipScreenVertically();
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(6, 54, String(buff));
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  //String temp =currTemp +"°C";
  display->drawString(128, 50,";)");
  display->drawHorizontalLine(0, 52, 128);
}

void setReadyForWeatherUpdate() {
  readyForWeatherUpdate = true;
}

void LightSwitchCallback(JsonVariant L){
    int LightSwitch = L["LightSwitch"];
    if (LightSwitch == 1)
    {
      //Serial.println("Turned ON");
      
        myservo.write(0);    
        delay(520);
        myservo.write(90);
       
    }
    else if (LightSwitch == 0)
    {
      //Serial.println("Turned OFF");
      //myservo.write(); 
        myservo.write(180);   
        delay(520);
        myservo.write(90);  

    }
}
/*
void AC_PowerCallback(JsonVariant L){
    int AC_Power = L["AC_Power"];
    if (AC_Power == 1){
      Serial.println("AC ON");
      ac.next.power = true;
      ACSEND
    }
    else if (AC_Power == 0){
      Serial.println("AC OFF");
      ac.next.power = false;
      ACSEND
    }
}

void TempSetCallback(JsonVariant p){
    int TempSet = p["TempSet"];
    Serial.print("Tempreture Set to: "); 
    Serial.print(TempSet);
    Serial.println("");
    setTemp=TempSet;
    ac.next.degrees = setTemp; 
    ACSEND   

}

void FanSpeedCallback(JsonVariant p){
    int Level = p["FanSpeed"];
    switch (Level)
    {
     case 0:
      ac.next.fanspeed = stdAc::fanspeed_t::kAuto;
      ACSEND
     Serial.println("AUTO");
     break;
     case 1:
      ac.next.fanspeed = stdAc::fanspeed_t::kLow;
      ACSEND
     Serial.println("LOW");
     break;
     case 2:
      ac.next.fanspeed = stdAc::fanspeed_t::kMedium;
      ACSEND
      Serial.println("MEDIUM");
     break;
     case 3:
      ac.next.fanspeed = stdAc::fanspeed_t::kHigh;
      ACSEND
      Serial.println("HIGH");
     break;
     default:
     break;
    }  
}

void SleepModeCallback(JsonVariant L){
  
int SleepMode=L["SleepMode"];
    
    if (SleepMode==1) {  
        ac.next.quiet = true;
        ACSEND
        Serial.println("SleepMode ON");
    }
    if (SleepMode==0) {  
        ac.next.quiet = false;
        ACSEND
        Serial.println("SleepMode OFF");
    }
   
}

//auto
void AutoModeCallback(JsonVariant L){
        int Auto=L["Auto"]; 
        if(Auto == 1){
          ac.next.mode = stdAc::opmode_t::kAuto;
          ACSEND
          Serial.println("Auto");
         }
        
        
}
//cool
void CoolingModeCallback(JsonVariant L){
        int Cool=L["Cool"]; 
        if(Cool == 1){
          ac.next.mode = stdAc::opmode_t::kCool;
          ACSEND
          Serial.println("Cool");
         }
}
//dry
 void DryModeCallback(JsonVariant L){
       int Dry=L["Dry"]; 
        if(Dry == 1){
          ac.next.mode = stdAc::opmode_t::kDry;
          ACSEND
          Serial.println("Dry");
         }
}
//fan
void FanModeCallback(JsonVariant L){
        int Fan=L["Fan"]; 
        if(Fan == 1){
          ac.next.mode = stdAc::opmode_t::kFan;
          ACSEND
          Serial.println("Fan");
         }
}
//heat
void HeatModeCallback(JsonVariant L){  
       int Heat=L["Heat"]; 
        if(Heat == 1){
          ac.next.mode = stdAc::opmode_t::kHeat;
          ACSEND
          Serial.println("Heat");
         }
}*/  
