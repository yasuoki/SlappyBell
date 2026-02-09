//
// Created by yasuoki on 2026/02/06.
//

#ifndef SLAPPYBELL_FIRMWARE_BLE_CONNECTION_H
#define SLAPPYBELL_FIRMWARE_BLE_CONNECTION_H
#include "NimBLECharacteristic.h"
#include "NimBLEServer.h"
#include "transport.h"

static constexpr char SERVICE_UUID[] = "032b4ecc-f367-4e08-bfe5-0f42aa4e62c0";
static constexpr char CTRL_RX_UUID[] = "032b4ecc-f367-4e08-bfe5-0f42aa4e62c1";
static constexpr char CTRL_TX_UUID[] = "032b4ecc-f367-4e08-bfe5-0f42aa4e62c2";
static constexpr char DATA_RX_UUID[] = "032b4ecc-f367-4e08-bfe5-0f42aa4e62c3";

class Processor;

class BleTransport : public Transport, NimBLECharacteristicCallbacks, NimBLEServerCallbacks  {
private:
	NimBLECharacteristic* _ctrlTx = nullptr;
	NimBLECharacteristic* _ctrlRx = nullptr;
	NimBLEAdvertising* _adv = nullptr;

	void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
	void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override;
	void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override;

public:
	explicit BleTransport(Processor *processor);
	~BleTransport() override;
	bool init() override;
	void start() override;
	void stop() override;
	size_t available() override;
	size_t read(uint8_t *data, size_t len) override;
	size_t send(const uint8_t *data, size_t len) override;

};


#endif //SLAPPYBELL_FIRMWARE_BLE_CONNECTION_H