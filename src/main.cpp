#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  // You may need to install this library through the Arduino Library Manager
#include <WiFiUdp.h>
#include <NTPClient.h>

ESP8266WebServer server(80);

const char *ntpServerName = "pool.ntp.org";
const int utcOffsetInSeconds = 0; // Adjust this based on your time zone

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerName, utcOffsetInSeconds);

void handleRoot() {
  String message = "Hello, this is the root page! ";
  Serial.println("Root");
  message += timeClient.getFormattedTime();
  server.send(200, "text/plain", message);
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
  Serial.begin(115200);

  // Connect to WiFi using WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");

  Serial.println("Connected to WiFi");

  // Define server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/post", HTTP_POST, handlePost);

  // Start the server
  server.begin();
  Serial.println("HTTP server started");

  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
  timeClient.update();
}

void loop() {
  // Handle incoming client requests
  server.handleClient();
}