#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "QuadSpiderAction.h"
#include "html.h"

const int led = 13;
MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
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
      quad_action_cmd(payload);
      //Serial.printf("%s\r\n", payload);
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

void handleRoot()
{
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
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
        Serial.print(" connected : ");
        Serial.println(WiFi.localIP());
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
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  quad_setup();
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
    server.handleClient();
    webSocket.loop();
    quad_loop();
  }
}
