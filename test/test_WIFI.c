#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ====== AP 参数 ======
const char* ap_ssid = "zwang";
const char* ap_password = "12345678";  

WebServer server(80);

// ====== 网页处理函数 ======
void handleRoot() {

  String html = "<!DOCTYPE html>";
  html += "<html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Balance Car</title>";
  html += "</head><body>";
  html += "<h1>ESP32 Balance Car</h1>";
  html += "<p>System Online</p>";
  html += "<p>Status: OK</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {

  Serial.begin(115200);

  // 启动 AP 模式
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress IP = WiFi.softAPIP();

  Serial.println("AP Mode Started");
  Serial.print("IP Address: ");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.begin();

  Serial.println("Web Server Started");
}

void loop() {
  server.handleClient();
}
