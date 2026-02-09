#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "processor.h"
#include "transport_serial.h"
#include "transport_ble.h"

byte serialBuffer[RECV_BUFFER_SIZE];
bool serialConnectStatus = false;
Processor *processor;
SerialTransport *serialTransport;
BleTransport *bleTransport;

void setup() {
	processor = new Processor();
	processor->init();
	serialTransport = new SerialTransport(processor);
	serialTransport->init();
	bleTransport = new BleTransport(processor);
	bleTransport->init();
}

void loop() {
	uint32_t now = millis();
	if (Serial) {
		if (!serialConnectStatus) {
			processor->onSerialConnect(now, serialTransport);
			serialConnectStatus = true;
		}
	} else {
		if (serialConnectStatus) {
			processor->onSerialDisconnect(now, serialTransport);
			serialConnectStatus = false;
		}
	}

//	if (Serial) {
		size_t size = serialTransport->available();
		size_t freeSize = RECV_BUFFER_SIZE;
		if(size > freeSize)
			size = freeSize;
		if(size > 0) {
			size = serialTransport->read(serialBuffer, size);
			processor->onSerialDataArrive(now, serialTransport, serialBuffer, size);
		}
//	}
	processor->process(now);
}
