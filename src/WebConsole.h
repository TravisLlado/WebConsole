/*
 * WebConsole - a minimal browser terminal for ESP32 over a WebSocket
 *
 * Serves one small page (dark background, light monospace text, a command
 * line) and a WebSocket at /ws. Text printed to the console is buffered and
 * sent to every connected browser on the next loop() call; output is dropped
 * when no browser is connected.
 *
 * Threading: the line handler runs on the AsyncTCP task, so it should only
 * copy the line and let the main loop act on it. print() and loop() must be
 * called from the same task (the Arduino loop task).
 */

#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class WebConsole : public Print {
 public:
  using LineHandler = void (*)(const char* line, size_t len);

  // Serve the page at `path` and the WebSocket at /ws on `server`
  void begin(AsyncWebServer& server, const char* path, LineHandler onLine);

  // Send buffered output and drop closed clients. Call every loop.
  void loop();

  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t* data, size_t len) override;

 private:
  static constexpr size_t   BUFFER_SIZE         {1024};
  static constexpr uint32_t CLEANUP_INTERVAL_MS {1000};

  void flush();

  AsyncWebSocket _ws {"/ws"};
  LineHandler    _onLine {nullptr};
  char           _buf[BUFFER_SIZE];
  size_t         _len {0};
  unsigned long  _lastCleanupMs {0};
};
