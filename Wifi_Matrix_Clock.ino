/*
       
       Zegar internetowy iClock v5

         (c) AdamBla76@gmail.com

           wersja WeMos D1 Mini

*/

#include <stdio.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>      // ESP library for all WiFi functions
#include <WiFiClientSecure.h>
#include <SPI.h>              // Library for hardware or software driven SPI
#include <EEPROM.h>           // Library for handle teh EEPROM
#include <MD_Parola.h>        // Parola library to scroll and display text on the display (needs MD_MAX72xx library)  https  //github.com/MajicDesigns/MD_Parola
#include <MD_MAX72xx.h>       // Library to control the Maxim MAX7219 chip on the dot matrix module   https  //github.com/MajicDesigns/MD_MAX72XX
#include <DNSServer.h>        // used in AP mode (config mode) to connect direct to the web page 
#include <ESP8266WebServer.h> // used in AP mode (config mode)
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include "WiFiManager.h"      // Manage auto connect to WiFi an fall back to AP   //https  //github.com/tzapu/WiFiManager
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "Parola_Fonts_data.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define HWVersion 1.0
#define SWVersion 1.41  // ustawione DST + aktualizacja wielu bibliotek

// ID STACJI IMGW 12560 https  //danepubliczne.imgw.pl/api/data/synop  https  //danepubliczne.imgw.pl/api/data/synop/id/12560
Ticker HeartBeat;
 
std::unique_ptr<ESP8266WebServer> server;
const char* hostIMGW      = "danepubliczne.imgw.pl";
char heart                = 3;
uint8_t colonT[] = { 1, 0, 0, 0, 0, 0, 0 };  // colonT
uint8_t ms_old[] = {8, 14, 1, 14, 1, 14, 144, 168, 72};
uint8_t ms[] = {8, 15, 65, 46, 17, 15, 148, 170, 72}; 

const int   portIMGW      = 443;
String      urlIMGW       = "/api/data/synop/id/12560"; 
// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprintIMGW     = "56 F9 DA AF 11 8C 32 EA 46 3A 8C BC 68 3C 04 8F A5 40 F8 24";
const char* robomania_SHA1 = "FF CD C7 91 63 8F 48 F1 F2 D5 B6 C6 7F 25 7F F5 A5 30 35 73";
char station[20];
char vtime[3];
char vdate[12];
float temp;
float wind;
float winddir;
float humidity;
float rain;
float pressure;
boolean nightmode;

// Define global variables and const
uint8_t frameDelay    = 25;  // default frame delay value
uint8_t nPage         = 0;
String msg;
char curMessage[255];
char prevMessage[255];
float prevPressure;

int8_t timeZone = 1;
int8_t minutesTimeZone = 0;
bool wifiFirstConnected = false;

time_t prevDisplay = 0; // when the digital clock was displayed
time_t prevGetIMGW = 0;
 
// Define the number of 8x8 dot matrix devices and the hardware SPI interface
#define  MAX_DEVICES 4
#define  MAX_ZONES 1
#define  CS_PIN  D8 
#define  CONTROL_PIN D2 

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event

WiFiManager wifiManager;
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);  // HARDWARE SPI for dot matrix display
Adafruit_NeoPixel pix = Adafruit_NeoPixel(4, 5, NEO_GRB + NEO_KHZ800);

void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
  Serial.println("NTP client started!");
  NTP.begin("pool.ntp.org", timeZone, true);
  NTP.setInterval(5);
}


//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  sprintf(curMessage,"%s", "setup");
  P.displayZoneText   (0,curMessage, PA_CENTER, 0, 0, PA_PRINT , PA_NO_EFFECT  );
  P.displayAnimate();
}

void processSyncEvent (NTPSyncEvent_t ntpEvent) {
    if (ntpEvent) {
        NTP.setInterval (5);
        Serial.print ("Time Sync error: ");
        if (ntpEvent == noResponse) 
            Serial.println ("NTP server not reachable");
        
        else if (ntpEvent == invalidAddress)
            Serial.println ("Invalid NTP server address");
    } 
    else {
        NTP.setInterval (300);
        Serial.print ("Got NTP time: ");
        Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
    }
}



void handleRoot() {
  server->send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";
  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->send(404, "text/plain", message);
}

void FirmwareUpdate(void)
{
      ESPhttpUpdate.rebootOnUpdate(false);
      t_httpUpdate_return rt = ESPhttpUpdate.update("https://robomania.pl/update.php",  ESP.getSketchMD5() ,robomania_SHA1);
      switch(rt) {
      case HTTP_UPDATE_FAILED:
          Serial.println("[checkForUpdates]: Update failed: "+String(ESPhttpUpdate.getLastErrorString()));
          break;
      case HTTP_UPDATE_NO_UPDATES:
          Serial.println("[checkForUpdates]: No Update needed");
          break;
      case HTTP_UPDATE_OK:
          Serial.println("[checkForUpdates]: Update ok."); // may not called we reboot the ESP
          Serial.flush();
          delay(2000);
          ESP.reset();
          break;
      }
}

void SetColor(uint8_t _r = 0, uint8_t _g = 0, uint8_t _b = 0)
{
   for(int i=0;i<4;i++){
    pix.setPixelColor(i,_r,_g,_b); // Moderately bright green color.
    pix.show(); // This sends the updated pixel color to the hardware.
  }
}

void ISRwatchdog(){
  //Serial.print("* ");
  ESP.wdtFeed();
}

void setup() 
{
  static WiFiEventHandler e1, e2;

  HeartBeat.attach(1,ISRwatchdog);
  Serial.begin(115200);
  Serial.println("zClock!");
  Serial.print("HW version: ");
  Serial.println(HWVersion);
  Serial.print("Soft version: ");
  Serial.println(SWVersion);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  EEPROM.begin(100); 

  NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
    if (ntpEvent) {
      Serial.print("Time Sync error: ");
      if (ntpEvent == noResponse)
        Serial.println("NTP server not reachable");
      else if (ntpEvent == invalidAddress)
        Serial.println("Invalid NTP server address");
    }
    else {
      Serial.print("Got NTP time: ");
      Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
    } 
  });

  
  pinMode(CONTROL_PIN, INPUT_PULLUP);  
  pix.begin();
  SetColor();
  byte i = EEPROM.read(0);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(90);
  nightmode=0;
  // Setup and start Dot matrix display
  P.begin(MAX_ZONES);
  P.setFont(fontPL);
  P.setZone(0, 0, 3);
  
  if(nightmode) {
    P.setIntensity(1);       // Values from 0 to 15
  }
  else
  {
    P.setIntensity(10);       // Values from 0 to 15
  }
  
  P.setSpeed(80);
  P.displayClear();  
  P.displaySuspend(false);
  P.addChar('^', colonT);
  P.addChar('~', ms);

  boolean Enter2Setup = 0;
  unsigned long WaitTime = millis();
  
  while ((millis() - WaitTime < 3000) && Enter2Setup==0)   // push button 3 sec to enter setup
  {
     if(digitalRead(CONTROL_PIN) == LOW) 
      { 
        Enter2Setup=1;
        Serial.println("Entering to setup ...");
      }
  }  

  if(Enter2Setup==1)
  {
   SetColor(0,255,0); 
   wifiManager.startConfigPortal("iClock","iClock");
   Serial.flush();
   ESP.reset();
   delay(5000);
  }
  else if(!wifiManager.autoConnect("iClock","iClock")) {          // Name of the buil-in accesspoint
   Serial.println("unable to connect - reset!");
   Serial.flush();
   delay(2000);
   ESP.reset();
   delay(4000);
  }
  else 
  {
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());
   wifiFirstConnected=1;
  }
  

  server.reset(new ESP8266WebServer(WiFi.localIP(), 80));
  server->on("/", handleRoot);
  server->on("/inline", []() {
    server->send(200, "text/plain", "this works as well");
  });

  server->onNotFound(handleNotFound);
  server->on("/hardreset.html", []() {
    server->send(200, "text/plain", "reset and reconfig");
    WiFi.disconnect();
    Serial.flush();
    delay(3000);
    ESP.reset();
    delay(1000);
  });
  server->begin();
  
  if (MDNS.begin( "iclock")) {
    MDNS.addService("http","tcp",80);
    Serial.println ( "MDNS responder started" );
  }

  
  NTP.begin("pool.ntp.org", timeZone, true);
  NTP.setInterval(5);
  Serial.println("NTP client started!");
  GetDataFromIMGW();

  sprintf(curMessage,"%s", "iCLK");
  P.displayZoneText   (0,curMessage, PA_CENTER, 50, 5000, PA_OPENING_CURSOR , PA_SCROLL_LEFT);
  
}


void loop() 
{
  static uint32_t  lastTime = 0;   
  static uint32_t  pressedTime = 0;  
  
  P.displayAnimate();

  if((digitalRead(CONTROL_PIN) == LOW) and (millis() - pressedTime >= 1000)) {
     pressedTime = millis();
     nPage=10; 
     prevDisplay=0;
  }
  
  if (wifiFirstConnected) {
        wifiFirstConnected = false;
        NTP.begin ("pool.ntp.org", timeZone, true);
        NTP.setInterval (5);
  }

  if (syncEventTriggered) {
        processSyncEvent (ntpEvent);
        syncEventTriggered = false;
  }

  server->handleClient();
  
  if (P.getZoneStatus(0) and (now()-prevDisplay>3))  // True if animation ended
  {
    switch (nPage) {

    case 0:
       sprintf(curMessage,"%02d%c%02d", hour(), (second() % 2 == 0 ? ':' : '^'), minute());
       P.displayZoneText(0,curMessage, PA_CENTER, 50, 0, PA_PRINT , PA_NO_EFFECT );
       P.setTextEffect(0, PA_SCROLL_UP  , PA_NO_EFFECT);
       P.displayReset(0);

      if ((hour()>22 or hour()<=6) and nightmode==0) {
        nightmode=1; 
        P.setIntensity(1);       // Values from 0 to 15  
      }

      if ((hour()>6) and (nightmode==1)) {
        nightmode=0; 
        P.setIntensity(10);       // Values from 0 to 15  
      }
      
      break;
      
    case 10:
      P.displayZoneText(0,curMessage, PA_CENTER, 40, 0, PA_PRINT , PA_NO_EFFECT );
      P.setTextEffect(0, PA_SCROLL_DOWN , PA_NO_EFFECT);
      sprintf(curMessage,"%d ", day());
      strcat(curMessage,monthShortStr(month()));
      P.displayReset(0);

      if (now() - prevGetIMGW > 2000) {   // co 33min pobieram dane z IMGW
          
          GetDataFromIMGW();
      }
      
      break;         
    
    case 11:
      P.displayZoneText(0,curMessage, PA_CENTER, 40, 0, PA_PRINT , PA_NO_EFFECT );
      P.setTextEffect(0, PA_SCROLL_UP  , PA_NO_EFFECT);
      sprintf(curMessage,"%d%cC", int(temp), char(247));
      P.displayReset(0);
      break;         

    case 12:
      P.displayZoneText(0,curMessage, PA_CENTER, 40, 0, PA_PRINT , PA_NO_EFFECT );
      P.setTextEffect(0, PA_SCROLL_DOWN  , PA_NO_EFFECT);
      sprintf(curMessage,"%d %% ", int(humidity));
      P.displayReset(0);
      break;         
    
    case 13:
      P.displayZoneText(0,curMessage, PA_CENTER, 40, 0, PA_PRINT , PA_NO_EFFECT );
      P.setTextEffect(0, PA_SCROLL_UP  , PA_NO_EFFECT);
      sprintf(curMessage,"%d h", int(pressure));
      if(prevPressure>pressure) curMessage[0]=24;
      else if(prevPressure<pressure) curMessage[0]=25;
      prevPressure = pressure;
      P.displayReset(0);
      break;         

    case 14:
      P.displayZoneText(0,curMessage, PA_CENTER, 40, 0, PA_PRINT , PA_NO_EFFECT );
      P.setTextEffect(0, PA_SCROLL_DOWN   , PA_NO_EFFECT);
      sprintf(curMessage,"%d %c", int(wind),'~');
      P.displayReset(0);
      break;
               
    case 15:
      P.displayZoneText(0,curMessage, PA_CENTER, 40, 0, PA_PRINT , PA_NO_EFFECT );
      P.setTextEffect(0, PA_SCROLL_UP  , PA_NO_EFFECT);
      sprintf(curMessage,"%d mm", int(rain));
      P.displayReset(0);
      break;

    case 16:
      FirmwareUpdate();
      break;
     
    }
    
    prevDisplay=now();
    strcpy(prevMessage,curMessage);
    Serial.print("[");Serial.print(nPage);Serial.print("] ");
    Serial.println(curMessage);
    if (++nPage>16){nPage=0;}
 
  }

  if (nPage>0 and nPage<10 and (millis() - lastTime >= 1000))
  {
    lastTime = millis();
    
    sprintf(curMessage,"%02d%c%02d", hour(), (second() % 2 == 0 ? ':' : '^'), minute());  
    P.displayZoneText(0,curMessage, PA_CENTER, 0, 0, PA_PRINT , PA_NO_EFFECT );
    P.setTextEffect(0, PA_PRINT , PA_NO_EFFECT);
    P.displayReset(0);
    
  }

 
  
}


void SetIntensity(String lumos ){
  long l = lumos.toInt();
  byte i = (byte) l;
  if ((i>=0) && (i<=15)){  
    EEPROM.write(0, i);    
    EEPROM.commit();
    P.setIntensity(i);   
  } 
}
  

void GetDataFromIMGW(){
  char endOfHeaders[] = "\r\n\r\n";
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 500;
  DynamicJsonDocument doc(capacity);
  
  WiFiClientSecure client;
  
  if (!client.connect(hostIMGW, portIMGW)) {
    Serial.println("connection failed");
    return;
  }   

  if (client.verify(fingerprintIMGW, hostIMGW)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  // This will send the request to the IMGW server
  client.print(String("GET ") + urlIMGW + " HTTP/1.1\r\n" +
               "Host: " + hostIMGW + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  client.find(endOfHeaders);
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  JsonObject root = doc.as<JsonObject>();

    strcpy(vdate,root["data_pomiaru"]); 
    strcpy(vtime,root["godzina_pomiaru"]); 
    strcpy(station,root["stacja"]); 
    Serial.print("Aktualne dane IMGW dla stacji "); Serial.print(station); Serial.print(" na ");
    Serial.print(vdate); Serial.print(" godz: "); Serial.println(vtime);
    Serial.println("-----------------------------------------------------------------------");
    Serial.print("Temp: ");               temp = atof(root["temperatura"]); Serial.println(temp);
    Serial.print("Ciśnienie: ");      pressure = atof(root["cisnienie"]); Serial.println(pressure);
    Serial.print("Wilgotność: ");     humidity = atof(root["wilgotnosc_wzgledna"]); Serial.println(humidity);
    Serial.print("Wiatr: ");              wind = atof(root["predkosc_wiatru"]); Serial.println(wind);
    Serial.print("Kierunek wiatru: "); winddir = atof(root["kierunek_wiatru"]); Serial.println(winddir);
    Serial.print("Opad: ");               rain = atof(root["suma_opadu"]); Serial.println(rain);   
    prevPressure=pressure;
    prevGetIMGW = now();
    if (temp < 19) 
     SetColor(0,0,50);  // cold blue
    else if(temp >=19 and temp <=22)
     SetColor(0,255,0);  // green
    else if(temp >22 and temp <=26)
     SetColor(255,220,0);  // yellow
    else 
     SetColor(255,0,0);  // red
  
  
}

void GetMessage(){
  // Use WiFiClient class to create TCP connections
  const char* host      = "robomania.pl";        // Here coms your Webserver / Host adress
  String url            = "/iClock.txt";         // The Path and Filename
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }   
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
     if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
     }
  }
  // Read all the lines of the reply from server
  while(client.available()){
      msg = client.readStringUntil('\r');
      Serial.print(msg);                             // Diag, Show's all the lines from HTTP answere
  }
}


