#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "ESP32 AP OK");
}

void setup() {
  Serial.begin(115200)

  WiFi.mode(WIFI_AP);

  bool ok = WiFi.softAP("BalanceCar", NULL, 1, 0);

  if(ok)
    Serial.println("AP Started OK");
  else
    Serial.println("AP FAILED");

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
}
