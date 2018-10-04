#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
//#include <ESP8266WebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "QuadSpiderAction.h"
#include "html.h"

const int led = 13;
MDNSResponder mdns;
/*
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
*/
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
#if MAX_SSID == 1
const char *ssid[MAX_SSID] = {_SSID1_};
const char *password[MAX_SSID] = { _WIFI_PASSWORD1_};
#endif

#if MAX_SSID == 2
const char *ssid[MAX_SSID] = {_SSID1_, _SSID2_};
const char *password[MAX_SSID] = { _WIFI_PASSWORD1_, _WIFI_PASSWORD2_};
#endif

#if MAX_SSID == 3
const char *ssid[MAX_SSID] = {_SSID1_, _SSID2_, _SSID3_};
const char *password[MAX_SSID] = { _WIFI_PASSWORD1_, _WIFI_PASSWORD2_, _WIFI_PASSWORD3_};
#endif

//=================================================================================

void ICACHE_FLASH_ATTR onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_ERROR) {
    Serial.printf("[ WARN ] WebSocket[%s][%u] error(%u): %s\r\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
    return;
  }
  if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      //the whole message is in a single frame and we got all of it's data
      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
        Serial.println(msg);
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
    }
  }
}

void ICACHE_FLASH_ATTR setupWebServer() {
  // Start WebSocket Plug-in and handle incoming message on "onWsEvent" function
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);
  // Handle what happens when requested web file couldn't be found
  server.onNotFound([](AsyncWebServerRequest * request) {
    AsyncWebServerResponse * response = request->beginResponse(404, "text/plain", "Not found");
    request->send(response);
  });
    
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });
  server.rewrite("/", "/index.html");
  // Start Web Server
  server.begin();
}

/*
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  //Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      //Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
      }
      break;
    case WStype_TEXT:
      Serial.printf("%s\r\n", payload);
      quad_action_cmd(payload);
      digitalWrite(LED_BUILTIN, LOW);
      // send data to all connected clients
      webSocket.broadcastTXT(payload, length);
      break;
    case WStype_BIN:
      hexdump(payload, length);
      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
      default:
      break;
  }
}
*/
void enableOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

int status = WL_IDLE_STATUS;
int maxQualityId;

bool connectWifi() {
  Serial.println("Connecting...");
	int n = WiFi.scanNetworks();
  maxQualityId = -1;
  int maxQuality = -1;
 	if (n == 0) {
 		return false;
	} else {
		for (int i = 0; i < n; ++i) {
		  for (int t = 0; t < MAX_SSID; t++) {
		    if (WiFi.SSID(i) == String(ssid[t])) {
    			int quality=0;
    			if (WiFi.RSSI(i) <= -100) {
    				quality = 0;
    			} else {
    				if (WiFi.RSSI(i) >= -50) {
    					quality = 100;
    				} else {
    					quality = 2 * (WiFi.RSSI(i) + 100);
    				}
    			}
    			if (quality > maxQuality) {
    			  maxQuality = quality;
    			  maxQualityId = t;
    			}
		    }
		  }
		}
		// try to connect to the max quality known network
		if (maxQualityId != -1) {
      Serial.print("Trying ");
      Serial.print(ssid[maxQualityId]);
      status = WiFi.begin(ssid[maxQualityId], password[maxQualityId]);
      int retries = 0;
      while (((status = WiFi.status()) != WL_CONNECTED) && (retries < 20)) {
        retries++;
        delay(1000);
        Serial.print(".");
      }
      if (status != WL_CONNECTED) {
        Serial.println(" failed");
        return false;
      } else {
        return true;
      }
		}
 	}
}

void WiFiEvent(WiFiEvent_t event) {
    switch(event) {
      
      case WIFI_EVENT_STAMODE_DISCONNECTED:
        Serial.println("WiFi lost connection: reconnecting...");
        connectWifi();
        //WiFi.begin();
        break;
      case WIFI_EVENT_STAMODE_CONNECTED:
        Serial.print("Connected to Wifi");
        break;
      case WIFI_EVENT_STAMODE_GOT_IP:
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        if (MDNS.begin("esp8266-amg8833")) {
          Serial.println("MDNS responder started");
        }
        enableOTA();
        break;
    }
}

void setup() {
  Serial.begin(115200);
  /* You can remove the password parameter if you want the AP to be open. */
#if 1
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiEvent);
  connectWifi();
  //WiFi.begin(ssid, password);
#else
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  if (mdns.begin("espWebSock", WiFi.localIP())) {
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
#endif  
  setupWebServer();
/*
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
*/  
  //quad_setup();
}

void loop() {
  
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long last_ms;
    unsigned long t = millis();
    if (t - last_ms > 500) {
      Serial.print(".");
      last_ms = t;
    }
  } else {
    ArduinoOTA.handle();
    //server.handleClient();
    //webSocket.loop();
    //quad_loop();
  }
}
