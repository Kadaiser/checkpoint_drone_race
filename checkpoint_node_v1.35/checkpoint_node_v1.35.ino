/********************************************************************************************
 *                                   RACE VARS
 *******************************************************************************************/
const int numContestants = 4;
String knownAddresses[numContestants] = {};
int foundAddresses[numContestants]    = {};
unsigned int checkpointNumber;
bool checkpointAssigned;
IPAddress deviceIP;

/********************************************************************************************
 *                                    TIMMERS
 *******************************************************************************************/
int totalInterruptCounter;
hw_timer_t * timerContestant1 = NULL;
hw_timer_t * timerContestant2 = NULL;
hw_timer_t * timerContestant3 = NULL;
hw_timer_t * timerContestant4 = NULL;
portMUX_TYPE timerMux1 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE timerMux2 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE timerMux3 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE timerMux4 = portMUX_INITIALIZER_UNLOCKED;

hw_timer_t * timerScreen = NULL; //create hardware timer
void startTimer(int hw_timmer_numer, hw_timer_t * timer, void (*)()); //2º argument as function
void endTimer1();
void endTimer2();
void endTimer3();
void endTimer4();

/********************************************************************************************
 *                                    SCHEDULER
 *******************************************************************************************/ 
#include <TaskScheduler.h>
Scheduler userScheduler; // to control your personal task

/********************************************************************************************
 *                                    MATRIZ LEDS
 *******************************************************************************************/ 
#include <SPI.h>
#include "LedMatrix.h"
#define NUMBER_OF_DEVICES 1 //number of led matrix connect in series
#define CS_PIN 12
#define CLK_PIN 14
#define MISO_PIN 1 //we do not use this pin just fill to match constructor
#define MOSI_PIN 13
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

void scrollText(String text);

/********************************************************************************************
 *                                    WIFI
 *******************************************************************************************/ 
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
const char* ssid     = "dronrace";
const char* password = "4thewin!";

/********************************************************************************************
 *                                    COAP
 *******************************************************************************************/ 
#include <WiFiUdp.h>
#include <coap-simple.h>
WiFiUDP udp;
Coap coap(udp);
void callback_response(CoapPacket &packet, IPAddress ip, int port);
void callback_led(CoapPacket &packet, IPAddress ip, int port);
void callback_setCheckpointPosition(CoapPacket &packet, IPAddress ip, int port);

//byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
//IPAddress ip(XXX,XXX,XXX,XXX);

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
     //calulate register runner and minimal time elapsed have endend for this addrress
     if (strcmp(address.c_str(), knownAddresses[i].c_str()) == 0 && foundAddresses[i] ==0)
     {
      //calculate valid distance
      int rssi = advertisedDevice.getRSSI();
      if (rssi > CUTOFF)
      {
        Serial.printf("Runner: ");
        Serial.println(address);
        foundAddresses[i] = 1;
        switch (i) {
          case 0:
            startTimer(0,timerContestant1, endTimer1);
            break;
          case 1:
            startTimer(1,timerContestant2, endTimer2);
            break;
          case 2:
            startTimer(2,timerContestant3, endTimer3);
            break;
          case 3:
            startTimer(3,timerContestant4, endTimer4);
            break;
          default:
            break;
        }
        scrollText(String(++i));
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
  
  ledMatrix.init();
  //ledMatrix.setIntensity(4); // range is 0-15
  ledMatrix.clear();
  ledMatrix.commit();
  

  //inicializate WEbServer
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.enableSTA(true);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {delay(500);}
  deviceIP = WiFi.localIP();
  Serial.println(WiFi.localIP());
  
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
  checkpointAssigned = false;
  scrollText("SET"); //22
}


/********************************************************************************************
 *                                         LOOP
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
  if(!checkpointAssigned)scrollText(String(deviceIP[3]));
}

/********************************************************************************************
 *                                    COAP FUNCTIONS
 *******************************************************************************************/ 
int sendPutToAddress(String ipAddress, int port,String resource, String msg)
{
  //return coap.put(IPAddress(10, 0, 0, 1), 5683, resource, msg);
  //return coap.get(IPAddress(10, 0, 0, 1), 5683, ,msg);
  //return coap.put(IPAddress(10, 0, 0, 1), port, resource, msg);
}

 
void callback_led(CoapPacket &packet, IPAddress ip, int port) {  

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  
  String message(p);
      
  if (message.equals("0")) {
    scrollText("V");
    coap.sendResponse(ip, port, packet.messageid, "1");
  } else if(message.equals("1")) { 
    scrollText("X");
    coap.sendResponse(ip, port, packet.messageid, "0");
  }
}

void callback_registerRunner(CoapPacket &packet, IPAddress ip, int port)
{ 
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  String message(p);
  Serial.printf("Register runner:");
  Serial.println(p);
  int i = 0;
  String resp= "ERROR";
  for(i; i < numContestants; i++)
  {
    if(knownAddresses[i] == 0)
    {
      knownAddresses[i] = String(p);
      foundAddresses[i] = 0;
      resp = String(++i);
      break;
    }
  }
  coap.sendResponse(ip, port, packet.messageid, "0");
  scrollText(resp);//12
}
void callback_setCheckpointPosition(CoapPacket &packet, IPAddress ip, int port)
{ 
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  String message(p);
  int newCheckPointValue = int(p[0]);
  Serial.println(newCheckPointValue);
  if(newCheckPointValue > 0)
  {
    checkpointNumber = newCheckPointValue;
    checkpointAssigned = true;
    scrollText("READY");
  }
  coap.sendResponse(ip, port, packet.messageid, "0");
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

void startTimer(int hw_timmer_numer,hw_timer_t * timer, void endTimer())
{
  Serial.println("Iniciando temporizador");
  timer = timerBegin(hw_timmer_numer, 80, true); // timer_id = 0; divider=80; countUp = true;
  timerAttachInterrupt(timer, endTimer, true); // edge = true
  timerAlarmWrite(timer, 3000000, false);  //1000 ms
  timerAlarmEnable(timer);
}

void IRAM_ATTR endTimer1() {
      portENTER_CRITICAL_ISR(&timerMux1);
      Serial.println("Finalizando temporizador");
      foundAddresses[0] = 0;
      portEXIT_CRITICAL_ISR(&timerMux1);
}
void IRAM_ATTR endTimer2() {
    portENTER_CRITICAL_ISR(&timerMux2);
    foundAddresses[1] = 0;
    timerContestant2 = NULL;
    portEXIT_CRITICAL_ISR(&timerMux2);
}
void IRAM_ATTR endTimer3() {
    portENTER_CRITICAL_ISR(&timerMux3);
    foundAddresses[2] = 0;

    timerContestant3 = NULL;
    portEXIT_CRITICAL_ISR(&timerMux3);
}
void IRAM_ATTR endTimer4() {
    portENTER_CRITICAL_ISR(&timerMux4);
    foundAddresses[3] = 0;
    timerContestant4 = NULL;
    portEXIT_CRITICAL_ISR(&timerMux4);
}

/********************************************************************************************
 *                                    LED MATRIX  FUNCTIONS
 *******************************************************************************************/

 void scrollText(String text)
 {
   unsigned int multiplier = 11;
  
   unsigned int textLength = text.length()*multiplier;
   
   ledMatrix.setText(text);
   for(int j = 0; j < textLength; j++)
   {
    ledMatrix.clear();
    ledMatrix.scrollTextLeft();
    ledMatrix.drawText();
    ledMatrix.commit();
    delay(50); //este scroll de delay habra que hacero en un timmer o una tarea asincrona.
   } 
   ledMatrix.clear();
   ledMatrix.commit();
 }
