#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  // You may need to install this library through the Arduino Library Manager
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include<Servo.h>
#include<PubSubClient.h>

ESP8266WebServer server(80);
StaticJsonDocument<200> doc;

const char *ntpServerName = "pool.ntp.org";
const int utcOffsetInSeconds = 0; // Adjust this based on your time zone

const char *mqtt_server = "192.168.1.140";
const int mqtt_port = 1883;

// Replace this with your MQTT topics
const char *state_topic = "central_heating/state";
const char *command_topic = "central_heating/cmd";

Servo myservo;
int threshold = 800;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerName, utcOffsetInSeconds);
int boost_duration = 30;
int led = 4, servo_start = 0, servo_end = 180, servo_delay = 1000;
boolean ledison = false;

WiFiClient espClient;
PubSubClient client(espClient);

bool checkIsOn(){
  Serial.println(analogRead(A0));
  return analogRead(A0) < threshold;
}

void toggle(bool On){
  for (int i = 0; i < 3; i++) {
    if (On != checkIsOn()){
      Serial.print("Turning - ");
      Serial.println(On ? "On" : "Off");
      //TODO - Toggle servo here
      myservo.write(servo_end);
      delay(servo_delay);
      myservo.write(servo_start);
      delay(servo_delay);
    }
  }
  doc["is_on"] = checkIsOn();
  String cmd = doc["is_on"] ? "ON" : "OFF";
  client.publish(state_topic, cmd.c_str());
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

void connectToMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("Connected to MQTT");
      // Subscribe to the command topic
      client.subscribe(command_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  // Handle messages received on the command topic
  payload[length] = '\0'; // Null-terminate the payload
  String message = String((char *)payload);

  Serial.print("Received message on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(message);

  // Add your logic to handle the received command
  // For example, you could toggle an LED based on the command
  // For simplicity, let's assume a "toggle" command
  if (message.equals("ON")) {
    // Add your code here to toggle something
    toggle(true);
  } else {
    toggle(false);
  }
}

void setup() {
  // Start Serial for debugging purposes
  
  doc["boost_start"] = 0;
  doc["is_on"] = false;
  doc["boosting"] = false;
  doc["boost_remaining"]=0;

  myservo.attach(5);
  myservo.write(servo_start);

  Serial.begin(115200);

  // Connect to WiFi using WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect("Heating Controller");

  Serial.println("Connected to WiFi");
  pinMode(A0, INPUT_PULLUP);
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

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Connect to MQTT
  connectToMQTT();
}

int loopcount = 0;

void loop() {
  // Handle incoming client requests
  server.handleClient();
  if(loopcount % 100  == 0){
    updateBoost();
    bool temp_on = checkIsOn();
    
    String cmd = temp_on ? "ON" : "OFF";
    client.publish(state_topic, cmd.c_str());
    
    doc["is_on"] = temp_on;
  }
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();
  loopcount++;
  delay(10);
}