#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "processor.h"

byte serialBuffer[RECV_BUFFER_SIZE];
bool serialConnectStatus = false;
Processor *processor;

void setup() {
	Serial.begin(115200);
	processor = new Processor();
	processor->init();
}

void loop() {
	uint32_t now = millis();
	if (Serial) {
		if (!serialConnectStatus) {
			processor->onSerialConnect(now);
			serialConnectStatus = true;
		}
	} else {
		if (serialConnectStatus) {
			processor->onSerialDisconnect(now);
			serialConnectStatus = false;
		}
	}

//	if (Serial) {
		int size = Serial.available();
		size_t freeSize = RECV_BUFFER_SIZE;
		if(size > freeSize)
			size = freeSize;
		if(size > 0) {
			Serial.read(serialBuffer, size);
			processor->onSerialDataArrive(now, serialBuffer, size);
		}
//	}
	processor->process(now);
}
