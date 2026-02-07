//
// Created by yasuoki on 2026/02/06.
//
#include <Arduino.h>
#include "config.h"
#ifndef SLAPPYBELL_FIRMWARE_TRANSPORT_H
#define SLAPPYBELL_FIRMWARE_TRANSPORT_H

enum TransportType {
	SERIAL_TRANSPORT,
	BLE_TRANSPORT
};

class Transport {
protected:
	TransportType type;
	Transport(TransportType type) : type(type) {}
	virtual ~Transport() = default;

	virtual size_t send(const uint8_t* data, size_t len) = 0;
	virtual size_t available() = 0;
	virtual size_t read(uint8_t* data, size_t len) = 0;
public:
	size_t printf(const char * format, ...);
	size_t send(const char* text);
};

#endif //SLAPPYBELL_FIRMWARE_TRANSPORT_H