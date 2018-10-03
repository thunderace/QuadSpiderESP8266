ARDUINO_VARIANT    = d1_mini
SERIAL_PORT = /dev/ttyUSB1
TAG = V0.1
FLASH_PARTITION=4M1M
#ULIBDIRS=./libraries
USER_LIBS=arduinoWebSockets
USER_DEFINE= -DMAX_SSID=2 -D_SSID1_=\"main\" -D_WIFI_PASSWORD1_=\"Delphine_Marie_Quentin_Arnaud\" -D_SSID2_=\"speedtouch\" -D_WIFI_PASSWORD2_=\"Delphine_Marie_Quentin_Arnaud\" -D_SSID3_=\"jardin\" -D_WIFI_PASSWORD3_=\"Delphine_Marie_Quentin_Arnaud\"
include $(HOME)/Esp8266-Arduino-Makefile/espXArduino.mk
myota: $(BUILD_OUT)/$(TARGET).bin
	node $(HOME)/repo.git/domo.mqtt/utils/sendOTA.js -n $(NODENAME)  -b $(BUILD_OUT)/$(TARGET).bin


