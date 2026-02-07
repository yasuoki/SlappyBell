//
// Created by yasuoki on 2026/02/06.
//

#include "transport_ble.h"
#include "NimBLEDevice.h"

BleTransport::BleTransport() : Transport(BLE_TRANSPORT) {
}

BleTransport::~BleTransport() {
}

bool BleTransport::init() {
	uint64_t mac = ESP.getEfuseMac(); // 48bit
	uint16_t tail = mac & 0xFFFF;

	char name[32];
	snprintf(name, sizeof(name), "SlappyBell-%04X", tail);
	NimBLEDevice::init(name);
	NimBLEServer* server = NimBLEDevice::createServer();

	NimBLEService* svc = server->createService(SERVICE_UUID);
	_ctrlTx = svc->createCharacteristic(
	  CTRL_TX_UUID, NIMBLE_PROPERTY::NOTIFY
	);
	_ctrlRx = svc->createCharacteristic(
	  CTRL_RX_UUID, NIMBLE_PROPERTY::WRITE
	);
	_ctrlRx->setCallbacks(this);
	svc->start();
	_adv = NimBLEDevice::getAdvertising();
	_adv->addServiceUUID(SERVICE_UUID);
	_adv->start();
}

size_t BleTransport::send(const uint8_t *data, size_t len) {
	if (!_ctrlTx) return 0;
	_ctrlTx->setValue(data, len);
	if (!_ctrlTx->notify())
		return 0;
	return len;
}

size_t BleTransport::available() {

}

size_t BleTransport::read(uint8_t *data, size_t len) {
}

void BleTransport::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
	NimBLEAttValue value = pCharacteristic->getValue();
	size_t len = value.size();
	const uint8_t *data = value.data();

	std::string v = pCharacteristic->getValue();
	String s(v.c_str());
	s.trim(); // \r\n対策
	if (s.length() == 0) return;
	handleCommandLine(s);
}

void BleTransport::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {
	_adv->stop();
}

void BleTransport::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
	_adv->start();
}
