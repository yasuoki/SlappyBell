//
// Created by yasuoki on 2026/02/06.
//

#include "transport_serial.h"

SerialTransport::SerialTransport(Processor* processor, Stream &stream) : Transport(SERIAL_TRANSPORT,processor), stream(stream) {
}

SerialTransport::~SerialTransport() {
}

bool SerialTransport::init()
{
	Serial.begin(115200);
	return true;
}

void SerialTransport::start()
{
}

void SerialTransport::stop()
{
}

size_t SerialTransport::available() {
	return stream.available();
}

size_t SerialTransport::read(uint8_t *data, size_t len) {
	return stream.readBytes(data, len);
}

size_t SerialTransport::send(const uint8_t *data, size_t len) {
	return stream.write(data, len);
}

