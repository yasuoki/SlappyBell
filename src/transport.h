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

class Processor;

class Transport {
protected:
	TransportType type;
	Processor* processor;
	Transport(TransportType type, Processor* processor) : type(type),processor(processor) {}
	virtual ~Transport() = default;
public:
	TransportType getType() { return type; }
	virtual bool init() = 0;
	virtual void close() = 0;
	virtual size_t available() = 0;
	virtual size_t read(uint8_t* data, size_t len) = 0;
	virtual size_t send(const uint8_t* data, size_t len) = 0;
	virtual void flush() = 0;
	size_t send(const char* text);
	size_t printf(const char * format, ...);
};

#endif //SLAPPYBELL_FIRMWARE_TRANSPORT_H