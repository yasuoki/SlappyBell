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
	char _deviceName[32];
	uint _mtuSize;
	NimBLEServer* _pServer;
	NimBLECharacteristic* _ctrlTx;
	NimBLECharacteristic* _ctrlRx;
	uint16_t _connHandle;
	volatile bool _indicateReady;

	bool (*_connectCallback)(Transport* transport);
	void (*_disconnectCallback)(Transport* transport);

	void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
	void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override;
	void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override;
	void onStatus(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, int code) override;
	bool waitSendingComplete();
public:
	explicit BleTransport(Processor *processor);
	~BleTransport() override;
	bool init() override;
	void close() override;
	size_t available() override;
	size_t read(uint8_t *data, size_t len) override;
	size_t send(const uint8_t *data, size_t len) override;
	void flush() override;
	void startAdv();
	void stopAvd();
	void setConnectCallback(bool (*cb)(Transport* transport));
	void setDisconnectCallback(void (*cb)(Transport* transport));
};


#endif //SLAPPYBELL_FIRMWARE_BLE_CONNECTION_H