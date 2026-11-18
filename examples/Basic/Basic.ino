// WebConsole basic example
//
// Prints a counter once per second and echoes back any line typed in the
// browser. Open http://<board IP>/ after it joins Wi-Fi.

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebConsole.h>

const char* WIFI_SSID     = "your-ssid";
const char* WIFI_PASSWORD = "your-password";

AsyncWebServer server(80);
WebConsole     console;

// The line handler runs on the AsyncTCP task: copy the line and handle it in
// loop(), so all output comes from one task.
portMUX_TYPE lineMux = portMUX_INITIALIZER_UNLOCKED;
char         pendingLine[64];
bool         hasPendingLine = false;

void onLine(const char* line, size_t len) {
  if (len >= sizeof(pendingLine)) len = sizeof(pendingLine) - 1;
  portENTER_CRITICAL(&lineMux);
  if (!hasPendingLine) {
    memcpy(pendingLine, line, len);
    pendingLine[len] = '\0';
    hasPendingLine = true;
  }
  portEXIT_CRITICAL(&lineMux);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(100);
  Serial.printf("Console: http://%s/\n", WiFi.localIP().toString().c_str());

  console.begin(server, "/", onLine);
  server.begin();
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    console.printf("uptime %lu s\n", lastPrint / 1000);
  }

  char line[sizeof(pendingLine)];
  bool have = false;
  portENTER_CRITICAL(&lineMux);
  if (hasPendingLine) {
    memcpy(line, pendingLine, sizeof(line));
    hasPendingLine = false;
    have = true;
  }
  portEXIT_CRITICAL(&lineMux);
  if (have) console.printf("you typed: %s\n", line);

  console.loop();
  delay(10);
}
