//
// Created by yasuoki on 2026/02/06.
//

#include "transport_serial.h"

SerialTransport::SerialTransport(Processor* processor) : Transport(SERIAL_TRANSPORT,processor) {
}

SerialTransport::~SerialTransport() = default;

bool SerialTransport::init()
{
	Serial.begin(115200);
	return true;
}

void SerialTransport::close()
{
	Serial.flush();
	Serial.end();
	vTaskDelay(100);
	init();
}

size_t SerialTransport::available() {
	return Serial.available();
}

size_t SerialTransport::read(uint8_t *data, size_t len) {
	return Serial.readBytes(data, len);
}

size_t SerialTransport::send(const uint8_t *data, size_t len) {
	return Serial.write(data, len);
}

void SerialTransport::flush()
{
	Serial.flush();
}

