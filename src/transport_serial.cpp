//
// Created by yasuoki on 2026/02/06.
//

#include "transport_serial.h"

SerialTransport::SerialTransport(Stream &stream) : Transport(SERIAL_TRANSPORT), stream(stream) {
}

SerialTransport::~SerialTransport() {
}

size_t SerialTransport::send(const uint8_t *data, size_t len) {
	return stream.write(data, len);
}

size_t SerialTransport::available() {
	return stream.available();
}

size_t SerialTransport::read(uint8_t *data, size_t len) {
	return stream.readBytes(data, len);
}
