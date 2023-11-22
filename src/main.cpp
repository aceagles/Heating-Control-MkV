#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  // You may need to install this library through the Arduino Library Manager
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);
StaticJsonDocument<200> doc;
const char *ntpServerName = "pool.ntp.org";
const int utcOffsetInSeconds = 0; // Adjust this based on your time zone

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerName, utcOffsetInSeconds);
int boost_duration = 30;

bool checkIsOn(){
  // TODO - Analogue read from photoresistor
  return true;
}

void toggle(bool On){
  if(doc["is_on"] != On){
  Serial.print("Turning - ");
  Serial.println(On ? "On" : "Off");

  // TODO - replace this with checkIsOn;
  doc["is_on"] = On;
  }
  if (On != checkIsOn()){
    //TODO - Toggle servo here
    
  }
}

void handleRoot() {
  String message = "Hello, this is the root page! ";
  Serial.println("Root");
  message += timeClient.getFormattedTime();
  server.send(200, "text/plain", message);
}

void startBoost() {
  doc["boosting"] = true;
  doc["boost_start"] = timeClient.getEpochTime();
  doc["boost_remaining"] = boost_duration;
  toggle(true);
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

void updateBoost(){
  if(doc["boosting"]){
    doc["boost_remaining"] = boost_duration - (timeClient.getEpochTime() - (unsigned long int)doc["boost_start"]);
    if (doc["boost_remaining"] <= 0){
      doc["boosting"] = false;
      doc["boost_remaining"] = 0;
      toggle(false);
    }
  }
  
}

void handleGet(){
  String command = server.arg("cmd");
  Serial.println(command);
  if (command == "On"){
    toggle(true);
  } else if (command == "Off" && !doc["boosting"])
  {
    toggle(false);
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

void handlePost() {
  String message = "Received POST request with JSON data:\n";

  if (server.hasArg("plain")) {
    String json = server.arg("plain");
    message += json;

    // Parse and process JSON data here
    Serial.print("post ");
    Serial.println(message);
    server.send(200, "text/plain", message);
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void setup() {
  // Start Serial for debugging purposes
  
  doc["boost_start"] = 0;
  doc["is_on"] = false;
  doc["boosting"] = false;
  doc["boost_remaining"]=0;

  Serial.begin(115200);

  // Connect to WiFi using WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");

  Serial.println("Connected to WiFi");

  // Define server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/post", HTTP_POST, handlePost);
  server.on("/data", HTTP_GET, handleGet);
  server.on("/boost", HTTP_GET, startBoost);

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  // Start the server
  server.begin();
  Serial.println("HTTP server started");

  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
  timeClient.update();
}
int loopcount = 0;
void loop() {
  // Handle incoming client requests
  server.handleClient();
  if(loopcount % 50 == 0)
    updateBoost();
  loopcount++;
  delay(10);
}