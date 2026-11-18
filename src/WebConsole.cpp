#include "WebConsole.h"

// ============================================================================
// Page
// ============================================================================

// Up/Down recall earlier commands. The log keeps the last 1000 messages.
static const char PAGE[] = R"html(<!doctype html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Console</title><link rel="icon" href="data:,">
<style>
html,body{height:100%;margin:0;background:#111;color:#ddd;font:14px/1.4 ui-monospace,Menlo,Consolas,monospace}
body{display:flex;flex-direction:column}
#log{flex:1;overflow-y:auto;margin:0;padding:8px;white-space:pre-wrap;overflow-wrap:anywhere}
#cmd{border:0;border-top:1px solid #333;background:#000;color:#ddd;font:inherit;padding:8px;outline:0}
</style></head><body>
<pre id="log"></pre>
<input id="cmd" autofocus autocomplete="off" autocapitalize="off" spellcheck="false" placeholder="connecting...">
<script>
const log=document.getElementById('log'),cmd=document.getElementById('cmd');
document.title=location.hostname;
const hist=[];let hi=0,ws;
function add(t){
  const atEnd=log.scrollHeight-log.scrollTop-log.clientHeight<8;
  log.append(t);
  while(log.childNodes.length>1000)log.firstChild.remove();
  if(atEnd)log.scrollTop=log.scrollHeight;
}
function connect(){
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen=()=>{cmd.placeholder="type a command"};
  ws.onmessage=e=>add(e.data);
  ws.onclose=()=>{cmd.placeholder="disconnected, retrying...";setTimeout(connect,1000)};
}
cmd.onkeydown=e=>{
  if(e.key==='Enter'){
    const v=cmd.value.trim();
    if(!v||!ws||ws.readyState!==1)return;
    add('> '+v+'\n');ws.send(v);
    if(hist[hist.length-1]!==v)hist.push(v);
    hi=hist.length;cmd.value='';
  }else if(e.key==='ArrowUp'&&hi>0){
    cmd.value=hist[--hi];e.preventDefault();
  }else if(e.key==='ArrowDown'&&hi<hist.length){
    cmd.value=hist[++hi]||'';e.preventDefault();
  }
};
connect();
</script></body></html>
)html";

// ============================================================================
// WebConsole
// ============================================================================

void WebConsole::begin(AsyncWebServer& server, const char* path, LineHandler onLine) {
  _onLine = onLine;

  server.on(path, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", reinterpret_cast<const uint8_t*>(PAGE), sizeof(PAGE) - 1);
  });

  // Runs on the AsyncTCP task
  _ws.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
                     void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      // A slow browser loses messages instead of being disconnected
      client->setCloseClientOnQueueFull(false);
      client->text("Connected.\n");
    } else if (type == WS_EVT_DATA && _onLine != nullptr) {
      // Commands are short, so only single-frame text messages are accepted
      const auto* info {static_cast<AwsFrameInfo*>(arg)};
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        _onLine(reinterpret_cast<const char*>(data), len);
      }
    }
  });
  server.addHandler(&_ws);
}

size_t WebConsole::write(const uint8_t* data, size_t len) {
  const size_t total {len};
  while (len > 0) {
    if (_len == BUFFER_SIZE) flush();
    const size_t n {min(len, BUFFER_SIZE - _len)};
    memcpy(_buf + _len, data, n);
    _len += n;
    data += n;
    len -= n;
  }
  return total;
}

void WebConsole::loop() {
  const unsigned long now {millis()};
  if (now - _lastCleanupMs >= CLEANUP_INTERVAL_MS) {
    _lastCleanupMs = now;
    _ws.cleanupClients();
  }
  flush();
}

void WebConsole::flush() {
  if (_len == 0) return;
  if (_ws.count() > 0) _ws.textAll(_buf, _len);
  _len = 0;
}
