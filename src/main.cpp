#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "processor.h"
#include "transport_serial.h"
#include "transport_ble.h"

byte serialBuffer[SERIAL_BUFFER_SIZE];
bool serialConnectStatus = false;
Processor *processor;
SerialTransport *serialTransport;
BleTransport *bleTransport;
//esp_reset_reason_t resetReason = ESP_RST_POWERON;

bool onBleConnect(Transport *transport)
{
	return processor->onTransportConnect(millis(), transport);
}
void onBleDisconnect(Transport *transport)
{
	processor->onTransportDisconnect(millis(), transport);
}

void setup() {
//	resetReason = esp_reset_reason();
	processor = new Processor();
	processor->init();
	serialTransport = new SerialTransport(processor);
	serialTransport->init();
	bleTransport = new BleTransport(processor);
	bleTransport->init();
	bleTransport->startAdv();
	bleTransport->setConnectCallback(onBleConnect);
	bleTransport->setDisconnectCallback(onBleDisconnect);
}

void loop() {
	uint32_t now = millis();
	/*
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
	*/
	if (!Serial)
	{
		if (serialConnectStatus) {
			processor->onSerialDisconnect(now, serialTransport);
			serialConnectStatus = false;
		}
	}
	/*
	if (Serial)
	{
		if (resetReason != ESP_RST_POWERON)
		{
			Serial.printf("Reset reason: %d\n",resetReason);
			resetReason = ESP_RST_POWERON;
		}
	}
	*/
	size_t size = Serial.available();
	if (size > 0)
	{
		if (!serialConnectStatus) {
			processor->onSerialConnect(now, serialTransport);
			serialConnectStatus = true;
		}
		if(size > sizeof(serialBuffer))
			size = sizeof(serialBuffer);
		size_t readSize =Serial.read(serialBuffer, size);
		processor->onTransportDataArrive(now, serialTransport, serialBuffer, readSize);
	}
	processor->process(now);
}
