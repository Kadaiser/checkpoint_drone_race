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
int bleLed = 19;
int wifiLed = 12;
bool wifiLedState;

/********************************************************************************************
 *                                    WIFI
 *******************************************************************************************/ 
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
const char* ssid     = "..............";
const char* password = "..............";
WebServer server(80);
void handleRoot();
void handleContestants();
void handleNotFound();

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
String knownAddresses[] = { "00:00:00:00:00:00" , "##:##:##:##:##:##"};
const int numContestants = sizeof(knownAddresses) / sizeof(*knownAddresses);
int foundAddresses[numContestants] = {0,0};
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

  //inicializate WEbServer
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {delay(500);}
  //create handlres
  server.on("/", handleRoot);
  server.on("/contestants", handleContestants);
  server.onNotFound(handleNotFound);
  server.begin();
  //Inicializate BLE scaner
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  //Inicializate CoAP callbacks
  coap.server(callback_led, "led");
  coap.response(callback_response);
  coap.start();
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
  server.handleClient();
  coap.loop();
}

/********************************************************************************************
 *                                    WEB SERVER
 *******************************************************************************************/ 
void handleRoot() {
}

void handleContestants()
{
  String message = "";
  for(int i = 1; i < numContestants; i++)
  {
    message += knownAddresses[i];
    message += "\n";
  }
  server.send(200, "text/plain", message);
}

void handleNotFound()
{
  server.send(404, "text/plain", "404");
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
    foundAddresses[1] = 0;
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
