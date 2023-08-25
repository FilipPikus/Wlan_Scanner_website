#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

const char* ssid = "Test";
const char* password = "1234";
WebServer server(80);

void setup() {
  Serial.begin(115200);


  WiFi.softAP(ssid, password);
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP: ");
  Serial.println(myIP);

  server.on("/", HTTP_GET, handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String html = "<html><body>";
  int numNetworks = WiFi.scanNetworks();
  
  if (numNetworks == 0) {
    html += "not Wlan found.";
  } else {
    html += "<ul>";
    for (int i = 0; i < numNetworks; ++i) {
      html += "<li>";
      html += WiFi.SSID(i);
      html += "</li>";
    }
    html += "</ul>";
  }
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}
