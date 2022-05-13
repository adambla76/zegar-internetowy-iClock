/////////////////////////////////////////////////////////
//
//                   iClock v1.1
//
/////////////////////////////////////////////////////////

// Include library
#include <stdio.h>
#include <ESP8266WiFi.h>      // ESP library for all WiFi functions
#include <WiFiClientSecure.h>
#include <SPI.h>              // Library for hardware or software driven SPI
#include <EEPROM.h>           // Library for handle teh EEPROM
#include <MD_Parola.h>        // Parola library to scroll and display text on the display (needs MD_MAX72xx library)  https  //github.com/MajicDesigns/MD_Parola
#include <MD_MAX72xx.h>       // Library to control the Maxim MAX7219 chip on the dot matrix module   https  //github.com/MajicDesigns/MD_MAX72XX
#include <DNSServer.h>        // used in AP mode (config mode) to connect direct to the web page 
#include <ESP8266WebServer.h> // used in AP mode (config mode)
#include <WiFiManager.h>      // Manage auto connect to WiFi an fall back to AP   //https  //github.com/tzapu/WiFiManager
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "Parola_Fonts_data.h"

// ID STACJI IMGW 12560 https  //danepubliczne.imgw.pl/api/data/synop  https  //danepubliczne.imgw.pl/api/data/synop/id/12560     ---    KOD DLA KATOWIC TO 12569 

const char* hostIMGW      = "danepubliczne.imgw.pl";
    
const int   portIMGW      = 443;
String      urlIMGW       = "/api/data/synop/id/12560"; 
// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprintIMGW = "20 87 9A C6 B1 A3 C8 86 10 DA 37 B1 00 44 A5 2F B2 F8 89 DF";

char station[20];
char vtime[3];
char vdate[12];
float temp;
float wind     = 0;
float winddir  = 0;
float humidity = 0;
float rain     = 0;
float pressure = 0;

// Define global variables and const
uint8_t frameDelay    = 25;  // default frame delay value
uint8_t nPage         = 0;
String msg;
char curMessage[255];
char prevMessage[255];
char mark;
float prevPressure = 0;

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
static const char ntpServerName[] = "0.pl.pool.ntp.org";
const int timeZone = 1;     // Central European Time
time_t prevDisplay = 0; // when the digital clock was displayed
time_t prevGetIMGW = 0;
 
// Define the number of 8x8 dot matrix devices and the hardware SPI interface
#define  MAX_DEVICES 8
#define  MAX_ZONES 2
#define  CS_PIN  D8 

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

WiFiManager wifiManager;
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);  // HARDWARE SPI for dot matrix display

void setup() 
{
  Serial.begin(115200);
  Serial.println("Start!");
  EEPROM.begin(100);   
  byte i = EEPROM.read(0);
 
  // Setup and start Dot matrix display
  P.begin(MAX_ZONES);
  P.setFont(fontPL);
  P.setZone(0, 0, 7);
  P.setIntensity(10);       // Values from 0 to 15
  P.setSpeed(80);
  P.displayClear();  
  P.displaySuspend(false);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(60);

  if (!wifiManager.autoConnect("iClock")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again
    ESP.reset();
    delay(2000);
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  Udp.begin(localPort);
  Serial.println("waiting for NTP server");
  setSyncProvider(getNtpTime);
  setSyncInterval(900);  

  sprintf(curMessage,"%s", "iClock v1.1");
  P.displayZoneText   (0,curMessage, PA_CENTER, 50, 5000, PA_OPENING_CURSOR , PA_NO_EFFECT  );
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.print("AP: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  //myWiFiManager->getConfigPortalSSID().toCharArray(station, sizeof(station));
  sprintf(curMessage, "  AP: %s ", station);
  //sprintf(curMessage, "  AP: %s  IP: %03d.%03d.%03d.%03d            ", station, WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
  //Serial.println(curMessage);
  //P.print(curMessage);
  //P.displayZoneText(0,curMessage, PA_CENTER, 50, 5000, PA_SCROLL_LEFT  , PA_SCROLL_LEFT );
  //P.displayReset(0);
  //while(!P.displayAnimate()) {} 
}


void loop() 
{
  if (P.displayAnimate() and P.getZoneStatus(0) and (now()-prevDisplay>3))  //  ekrany zmieniaaja się co 3 sek
  {
    switch (nPage) {

    case 0:   // pokazuje zegar
      sprintf(curMessage,"%02d:%02d", hour(), minute());
      P.setTextEffect(0, PA_SCROLL_UP  , PA_NO_EFFECT);
      P.displayReset(0);
      break;
    
    case 5:  // pobiera dane z IMGW
      if (now() - prevGetIMGW > 2000) {   // co 33min pobieram dane z IMGW
          prevGetIMGW = now();
          GetDataFromIMGW();
      }
     break;
     
    case 10:  // pokazuje bieżącą date
      P.setTextEffect(0, PA_SCROLL_DOWN , PA_NO_EFFECT);
      sprintf(curMessage,"%3s%d ", dayShortStr(weekday()), day());
      strcat(curMessage,monthShortStr(month()));
      P.displayReset(0);
      break;         
    
    case 11:  // pokazuje aktualna temperaturę i wilgotność
      P.setTextEffect(0, PA_SCROLL_UP  , PA_NO_EFFECT);
      sprintf(curMessage,"%d%cC   %d%%", int(temp), char(247), int(humidity));
      P.displayReset(0);
      break;         
    
    case 12:  // pokazuje aktualne ciśnienie atmo
      P.setTextEffect(0, PA_SCROLL_DOWN  , PA_NO_EFFECT);
      
      if(prevPressure>pressure) mark=char(24);
      else if(prevPressure<pressure) mark=char(25);
      else mark=' ';
      
      prevPressure = pressure;
      sprintf(curMessage,"%c %d hPa ", mark, int(pressure));
      P.displayReset(0);
      break;         

    case 13: // pokazuje prędkość wiatru
      P.setTextEffect(0, PA_SCROLL_UP   , PA_NO_EFFECT);
      sprintf(curMessage,"%d m/s", int(wind));
      P.displayReset(0);
      break;
               
    case 14:  // pokazuje opad atmo
      P.setTextEffect(0, PA_SCROLL_DOWN  , PA_NO_EFFECT);
      sprintf(curMessage,"opad %d mm", int(rain));
      P.displayReset(0);
      break;
  
    default:
      break;
    }
    
    prevDisplay=now();
    strcpy(prevMessage,curMessage);
    Serial.print("[");Serial.print(nPage);Serial.print("] ");
    Serial.println(curMessage);
    if (++nPage>14){nPage=0;}
 
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
  

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 3000) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void GetDataFromIMGW(){
  char endOfHeaders[] = "\r\n\r\n";

  StaticJsonBuffer<1000> jsonBuffer;
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

  // This will send the request to the server
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
  JsonObject& root = jsonBuffer.parseObject(client);

  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println(" JSON parse failed");
    return;
  }
  else {
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
  }
  
}


