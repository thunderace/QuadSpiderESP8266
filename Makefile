ARDUINO_VARIANT    = d1_mini
SERIAL_PORT = /dev/ttyUSB1
TAG = V0.1
FLASH_PARTITION=4M1M
#ULIBDIRS=./libraries
USER_LIBS=arduinoWebSockets
include $(HOME)/Esp8266-Arduino-Makefile/espXArduino.mk
myota: $(BUILD_OUT)/$(TARGET).bin
	node $(HOME)/repo.git/domo.mqtt/utils/sendOTA.js -n $(NODENAME)  -b $(BUILD_OUT)/$(TARGET).bin


