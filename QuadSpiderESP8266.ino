/* -----------------------------------------------------------------------------
  - Original Project: Body Movement Crawling robot
  - Original Author:  panerqiang@sunfounder.com
  - Date:  2015/1/27

  - Remix Project by Wilmar
  - Date: 2018/09/25

  - Remix Project by thunderace (for ESP8266 only)
  - Date 2018/10/10

/* Includes ------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <ArduinoOTA.h>
#include "QuadSpiderAction.h"
#include "html.h"

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

String command = "";
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
        if (command == "") { // ignore command if last one is not terminated
          Serial.println(msg);
          command = msg;
        } else {
          Serial.println("Last command not yet terminated : ignore this one!");
        }
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

void fallbackToAP() {
    WiFi.softAP("Spider");
}

int status = WL_IDLE_STATUS;
int maxQualityId;
bool connectWifi() {
  Serial.println("Connecting...");
	int n = WiFi.scanNetworks();
  maxQualityId = -1;
  int maxQuality = -1;
 	if (n == 0) {
    fallbackToAP();
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
        fallbackToAP();
        return false;
      } else {
        return true;
      }
		}
 	}
}

WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;

void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  Serial.print("Station connected ");
}

void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
  Serial.print("Station disconnected: ");
  connectWifi();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  stationConnectedHandler = WiFi.onSoftAPModeStationConnected(&onStationConnected);
  stationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);
  connectWifi();
  setupWebServer();
  quad_setup();
}

void loop() {
  if (command != "") {
    quad_action_cmd((unsigned char *)command.c_str());
    command = "";
  }
  quad_loop();
}
