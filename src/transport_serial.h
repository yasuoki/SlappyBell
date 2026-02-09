//
// Created by yasuoki on 2026/02/06.
//

#ifndef SLAPPYBELL_FIRMWARE_TRANSPORT_SERIAL_H
#define SLAPPYBELL_FIRMWARE_TRANSPORT_SERIAL_H
#include <Arduino.h>
#include "transport.h"

class Processor;

class SerialTransport : public Transport {
private:
	Stream &stream;
public:
	explicit SerialTransport(Processor *processor, Stream &stream = Serial);
	~SerialTransport() override;
	bool init() override;
	void start() override;
	void stop() override;
	size_t available() override;
	size_t send(const uint8_t *data, size_t len) override;
	size_t read(uint8_t *data, size_t len) override;
};


#endif //SLAPPYBELL_FIRMWARE_TRANSPORT_SERIAL_H