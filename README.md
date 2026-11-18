# WebConsole

A minimal browser terminal for ESP32. It serves one small page (about 2 KB, no external resources) with a dark background, light monospace text and a command line, and talks to it over a WebSocket using [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer).

- Output you `print()` shows up in every connected browser
- Lines typed in the browser are passed to your handler
- Up/Down recall earlier commands; the page reconnects by itself if the board resets
- The page keeps the last 1000 messages

## Install

PlatformIO:

```ini
lib_deps =
  https://github.com/TravisLlado/WebConsole.git#v1.0.0
```

ESPAsyncWebServer and AsyncTCP come along as dependencies.

## Use

```cpp
#include <ESPAsyncWebServer.h>
#include <WebConsole.h>

AsyncWebServer server(80);
WebConsole     console;

void onLine(const char* line, size_t len) {
  // Runs on the AsyncTCP task: copy the line and act on it in loop()
}

void setup() {
  // ... connect to Wi-Fi ...
  console.begin(server, "/", onLine);  // page at /, WebSocket at /ws
  server.begin();
}

void loop() {
  console.printf("hello %lu\n", millis());
  console.loop();  // sends buffered output
}
```

`WebConsole` is a `Print`, so `print`, `println` and `printf` all work. See [examples/Basic](examples/Basic/Basic.ino) for a complete sketch, including handing lines from the handler to `loop()`.

## How it works

- **Output** is buffered (1 KB) and sent as one WebSocket text message per `loop()` call, so a burst of prints becomes one message. Output is dropped while no browser is connected.
- **Input** arrives on the AsyncTCP task. The handler gets the line without a trailing newline and without a null terminator, so use `len`.
- **Threading:** call `print()` and `loop()` from the same task (normally the Arduino loop task). Keep the handler short and don't print from it.
- A slow browser loses messages instead of being disconnected. Closed clients are cleaned up once per second.

## Tip: slow `.local` lookups on macOS

If `http://<name>.local/` takes about 5 s to open, the board is probably answering mDNS for IPv4 only, and macOS is waiting for an IPv6 answer that never comes. Giving the station an IPv6 link-local address fixes it (arduino-esp32 2.x):

```cpp
WiFi.onEvent([](arduino_event_id_t, arduino_event_info_t) { WiFi.enableIpV6(); },
             ARDUINO_EVENT_WIFI_STA_CONNECTED);
WiFi.begin(ssid, password);
```

## License

MIT
