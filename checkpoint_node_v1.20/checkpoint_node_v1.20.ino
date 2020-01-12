/********************************************************************************************
 *                                   RACE VARS
 *******************************************************************************************/
const int numContestants = 3;
String knownAddresses[numContestants] = {};
int foundAddresses[numContestants]    = {};
unsigned int checkpointNumber;

/********************************************************************************************
 *                                    TIMMERS
 *******************************************************************************************/ 
hw_timer_t * timer = NULL; //create hardware timer
hw_timer_t * timer2 = NULL; //create hardware timer 
void onTimer();
void startTimer();
void endTimer();

/********************************************************************************************
 *                                    SCHEDULER
 *******************************************************************************************/ 
#include <TaskScheduler.h>
Scheduler userScheduler; // to control your personal task

/********************************************************************************************
 *                                    LEDS
 *******************************************************************************************/ 
#include <SPI.h>

#include "LedMatrix.h"
#define NUMBER_OF_DEVICES 1 //number of led matrix connect in series
#define CS_PIN 12
#define CLK_PIN 14
#define MISO_PIN 1 //we do not use this pin just fill to match constructor
#define MOSI_PIN 13
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

int bleLed = 23;
int wifiLed = 19;
bool wifiLedState;

 void scrollText(String text, int textLength);

/********************************************************************************************
 *                                    WIFI
 *******************************************************************************************/ 
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
const char* ssid     = "houseMesh";
const char* password = "Enestano@.n0p";

/********************************************************************************************
 *                                    COAP
 *******************************************************************************************/ 
#include <WiFiUdp.h>
#include <coap-simple.h>
WiFiUDP udp;
Coap coap(udp);
void callback_response(CoapPacket &packet, IPAddress ip, int port);
void callback_led(CoapPacket &packet, IPAddress ip, int port);

/********************************************************************************************
 *                                    BLE
 *******************************************************************************************/ 
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

int scanTime = 5;
BLEScan* pBLEScan;
const int CUTOFF = -50;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    String address = advertisedDevice.getAddress().toString().c_str();
    int i = 0;
    for(i; i < numContestants; i++)
    {
     //calulate minimal time elapsed have endend for this addrress
     if (strcmp(address.c_str(), knownAddresses[i].c_str()) == 0 && foundAddresses[i] ==0)
     {
      //calculate valid distance
      int rssi = advertisedDevice.getRSSI();
      if (rssi > CUTOFF)
      {
        Serial.printf("Runner: ");
        Serial.println(address);
        foundAddresses[i] = 1;
        digitalWrite(bleLed, HIGH);
        startTimer();
      }
     }
    }
    advertisedDevice.getScan()->stop();
  }
  
};

/********************************************************************************************
 *                                    MESH
 *******************************************************************************************/ 
/*
#include <painlessMesh.h>
#define   MESH_SSID       "..........."
#define   MESH_PASSWORD   "..........."
#define   MESH_PORT       5555
painlessMesh  mesh;
// MESH PROTOTIPES FUNCTIONS
void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void nodeTimeAdjustedCallback(int32_t offset); 
void delayReceivedCallback(uint32_t from, int32_t delay);
*/

/********************************************************************************************
 *                                    SETUP
 *******************************************************************************************/ 
void setup()
{
  Serial.begin(115200);
  pinMode(bleLed, OUTPUT);
  pinMode(wifiLed, OUTPUT);
  digitalWrite(bleLed, LOW);
  digitalWrite(wifiLed, LOW);
  
  ledMatrix.init();
  //ledMatrix.setIntensity(4); // range is 0-15
  ledMatrix.clear();
  ledMatrix.commit();
  

  //inicializate WEbServer
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {delay(500);}
  
  //Inicializate BLE scaner
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  //Inicializate CoAP callbacks
  coap.server(callback_led, "led");
  coap.server(callback_registerRunner, "reg_runner");
  coap.server(callback_setCheckpointPosition, "set_checkpoint");
  coap.response(callback_response);
  coap.start();
  scrollText("GO"); //22
}


/********************************************************************************************
 *                                    LOOP
 *******************************************************************************************/ 

void loop()
{
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  //Serial.print("Devices found: ");
  //Serial.println(foundDevices.getCount());
  //Serial.println("Scan done!");
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  //delay(2000);
  coap.loop();
}

/********************************************************************************************
 *                                    COAP FUNCTIONS
 *******************************************************************************************/ 
void callback_led(CoapPacket &packet, IPAddress ip, int port) {  

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  
  String message(p);

  if (message.equals("0"))
    wifiLedState = false;
  else if(message.equals("1"))
    wifiLedState = true;
      
  if (wifiLedState) {
    digitalWrite(wifiLed, HIGH) ; 
    coap.sendResponse(ip, port, packet.messageid, "1");
  } else { 
    digitalWrite(wifiLed, LOW) ; 
    coap.sendResponse(ip, port, packet.messageid, "0");
  }
}

void callback_registerRunner(CoapPacket &packet, IPAddress ip, int port)
{ 
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  String message(p);
  Serial.println("mensajeEntrate");
  Serial.println(p);
  int i = 0;
  for(i; i < numContestants; i++)
  {
    if(knownAddresses[i] == 0)
    {
      knownAddresses[i] = String(p);
      foundAddresses[i] = 0;
      break;
    }
  }
  scrollText(String(++i));//12
}
void callback_setCheckpointPosition(CoapPacket &packet, IPAddress ip, int port)
{ 
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  String message(p);
  Serial.println(p);
}


void callback_response(CoapPacket &packet, IPAddress ip, int port)
{
  Serial.println("[Coap Response got]");
  
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  Serial.println(p);
}


/********************************************************************************************
 *                                    TIMMER FUNCTIONS
 *******************************************************************************************/ 
void onTimer(){
    endTimer();
    digitalWrite(bleLed, LOW);
    foundAddresses[0] = 0;
    //ledMatrix.clear();
}

void startTimer()
{
  timer = timerBegin(0, 80, true); // timer_id = 0; divider=80; countUp = true;
  timerAttachInterrupt(timer, &onTimer, true); // edge = true
  timerAlarmWrite(timer, 3000000, false);  //1000 ms
  timerAlarmEnable(timer);
}

void endTimer() {
  timerEnd(timer);
  timer = NULL; 
}

/********************************************************************************************
 *                                    LED MATRIX  FUNCTIONS
 *******************************************************************************************/

 void scrollText(String text)
 {
   unsigned int textLength = text.length()*11;
   
   ledMatrix.setText(text);
   for(int j = 0; j < textLength; j++)
   {
    ledMatrix.clear();
    ledMatrix.scrollTextLeft();
    ledMatrix.drawText();
    ledMatrix.commit();
    delay(100); //este scroll de delay habra que hacero en un timmer o una tarea asincrona.
   } 
   ledMatrix.clear();
   ledMatrix.commit();
 }
