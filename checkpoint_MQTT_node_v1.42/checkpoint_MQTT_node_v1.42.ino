/********************************************************************************************
 *                                   RACE VARS
 *******************************************************************************************/
const int numContestants = 4;
String knownAddresses[numContestants] = {};
int foundAddresses[numContestants]    = {};
unsigned int checkpointNumber;
bool checkpointAssigned;
IPAddress deviceIP;
const char deviceName[] = "esp32_0A";

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
WiFiClient wifiClient;
const char* ssid     = "dronrace";
const char* password = "4thewin!";
void reconnect();

/********************************************************************************************
 *                                    MQTT
 *******************************************************************************************/
#include <PubSubClient.h>
#include <ArduinoJson.h>
const char* mqtt_server = "192.168.4.1";//dronerace.mqtt.com
#define mqtt_port 1883

PubSubClient client(wifiClient);
void callback(char* topic, byte* payload, unsigned int length);


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
        StaticJsonDocument<256> doc;
        doc["checkpoint"] = deviceName;
        doc["runner"] = knownAddresses[i];
        char buffer[512];
        size_t n = serializeJson(doc, buffer);
        client.publish("dronrace/detectedRunner", buffer, n);
        scrollText(String(i));
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
  scrollText("HI"); //22

  //WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  //WiFi.enableSTA(true);
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

  //Inicializate MQTT callbacks
  //client.setClient(wifiClient);
  client.setServer(mqtt_server, mqtt_port);
    while (!client.connected())
    {
    Serial.println("Connecting to MQTT...");
    if (client.connect(deviceName)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  client.setCallback(callback);
  client.subscribe("dronrace/setCheckpoint");
  client.subscribe("dronrace/regRunner");
  checkpointAssigned = false;
  scrollText("SET");
}


/********************************************************************************************
 *                                         LOOP
 *******************************************************************************************/ 

void loop()
{
  if ( WiFi.isConnected() ) 
  {
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

    client.loop();
    if(!checkpointAssigned)scrollText(String(deviceIP[3]));
  }
  else reconnect();
  
}

/********************************************************************************************
 *                                    MQTT FUNCTIONS
 *******************************************************************************************/ 

void callback(char* topic, byte* payload, unsigned int length)
{
  // handle message arrived
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);
  
  if (strcmp(topic,"dronrace/setCheckpoint")==0)
  {
    unsigned int value = doc["value"];
    checkpointNumber = value;
    checkpointAssigned = true;
  }
 
  if (strcmp(topic,"dronrace/regRunner")==0)
  {
    String runnerMAC = doc["runner"];
    bool avaliable = true;
    int i;
  
    String resp= "ERROR";
    //runner alrready register in this checkpoint
    for(i = 0; i < numContestants; i++)
    {
      if (strcmp(runnerMAC.c_str(), knownAddresses[i].c_str()) == 0) avaliable = false;
    }
 
    if(avaliable)//search empty space to include
      for(i = 0; i < numContestants; i++)
      {
        if(knownAddresses[i] == 0)
        {
          knownAddresses[i] = runnerMAC;
          foundAddresses[i] = 0;
          resp = String(++i);
          Serial.printf("Register runner:");
          Serial.println(runnerMAC);
          break;
        }
      }
    scrollText(resp);//12
  } 
}

/********************************************************************************************
 *                                    TIMMER FUNCTIONS
 *******************************************************************************************/ 

void startTimer(int hw_timmer_numer,hw_timer_t * timer, void endTimer())
{
  Serial.println("Iniciando temporizador");
  timer = timerBegin(hw_timmer_numer, 80, true); // timer_id = 0; divider=80; countUp = true;
  timerAttachInterrupt(timer, endTimer, true); // edge = true
  timerAlarmWrite(timer, 5000000, false);  //1000 ms
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
   unsigned int multiplier = 8;
   if(text.length() == 1) multiplier = 13;
   if(text.length() == 2) multiplier = 10;
   if(text.length() == 3) multiplier = 9;
   
  
   unsigned int textLength = text.length()*multiplier;
   
   ledMatrix.setText(text);
   for(int j = 0; j < textLength; j++)
   {
    ledMatrix.clear();
    ledMatrix.scrollTextLeft();
    ledMatrix.drawText();
    ledMatrix.commit();
    delay(30); //try to found a way without delay
   } 
   ledMatrix.clear();
   ledMatrix.commit();
 }
 
/********************************************************************************************
 *                                    WIFI Overwatch
 *******************************************************************************************/
 void reconnect()
 {
  Serial.println("Reconectando");
   //WiFi.persistent(false);
   WiFi.mode(WIFI_STA);
  // WiFi.enableSTA(true);
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
     scrollText("......");
    }
   deviceIP = WiFi.localIP();
  client.subscribe("dronrace/setCheckpoint");
  client.subscribe("dronrace/regRunner");
  client.publish("dronrace/reconnnections", deviceName);
 }
 
