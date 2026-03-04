//
// Created by yasuoki on 2026/02/06.
//

#include "transport_ble.h"
#include "NimBLEDevice.h"
#include "processor.h"
#include "esp_bt_main.h"           // Bluetoothスタックのヘッダ
#include "esp_bt_device.h"
BleTransport::BleTransport(Processor *processor) : Transport(BLE_TRANSPORT, processor)
{
	_deviceName[0] = '\0';
	_mtuSize = 0;
	_pServer = nullptr;
	_ctrlTx = nullptr;
	_ctrlRx = nullptr;
	_connHandle = 0;
	_indicateReady = true;

	_connectCallback = nullptr;
	_disconnectCallback = nullptr;
}

BleTransport::~BleTransport() = default;

bool BleTransport::init() {
	uint8_t mac[8];
	esp_read_mac(mac, ESP_MAC_BT);
	snprintf(_deviceName, sizeof(_deviceName), PRODUCT_NAME "-%02X%02X", mac[4], mac[5]);

	NimBLEDevice::init(_deviceName);
	NimBLEDevice::setMTU(BLE_MTU_SIZE);

	_pServer = NimBLEDevice::createServer();

	NimBLEService* svc = _pServer->createService(SERVICE_UUID);
	_ctrlTx = svc->createCharacteristic(
	  CTRL_TX_UUID, NIMBLE_PROPERTY::INDICATE
	);
	_ctrlRx = svc->createCharacteristic(
	  CTRL_RX_UUID, NIMBLE_PROPERTY::WRITE
	);
	_pServer->setCallbacks(this);
	_ctrlTx->setCallbacks(this);
	_ctrlRx->setCallbacks(this);
	svc->start();

	return true;
}

void BleTransport::close()
{
	if(_pServer->getConnectedCount() > 0)
		_pServer->disconnect(_connHandle);
	_connHandle = 0;
}

void BleTransport::startAdv()
{
	NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
	adv->stop();
	adv->reset();
	adv->addServiceUUID(SERVICE_UUID);

	NimBLEAdvertisementData ad;
	ad.setFlags(0x06);
	ad.addServiceUUID(SERVICE_UUID);
	adv->setAdvertisementData(ad);

	NimBLEAdvertisementData scan;
	scan.setName(_deviceName);
	adv->setScanResponseData(scan);
	adv->start();
}

void BleTransport::stopAvd()
{
	NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
	adv->stop();
}

bool BleTransport::waitSendingComplete()
{
	if (_indicateReady)
		return true;
	if (!_ctrlTx)
		return false;
	uint32_t t = millis();
	while (!_indicateReady && millis() - t < 1000)
		vTaskDelay(50);
	return _indicateReady;
}

size_t BleTransport::send(const uint8_t *data, size_t len) {
	if (!_ctrlTx)
	{
		return 0;
	}

	uint chunkSize = _mtuSize - 3;
	for (uint offset = 0; offset < len; offset += chunkSize)
	{
		if (!waitSendingComplete())
			return offset;
		_indicateReady = false;
		size_t sendSize = len - offset;
		if (sendSize > chunkSize) sendSize = chunkSize;
		_ctrlTx->setValue(data + offset, sendSize);
		if (!_ctrlTx->indicate())
		{
			return offset;
		}
	}
	return len;
}

void BleTransport::flush()
{
	waitSendingComplete();
}

void BleTransport::setConnectCallback(bool(* cb)(Transport* transport))
{
	_connectCallback = cb;
}

void BleTransport::setDisconnectCallback(void(* cb)(Transport* transport))
{
	_disconnectCallback = cb;
}

size_t BleTransport::available() {
	return 0;
}

size_t BleTransport::read(uint8_t *data, size_t len) {
	return 0;
}

void BleTransport::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {
	//stopAvd();
	_mtuSize = pServer->getPeerMTU(connInfo.getConnHandle());
	_connHandle = connInfo.getConnHandle();
	_indicateReady = true;
	if (_connectCallback != nullptr)
	{
		if (!_connectCallback(this))
		{
			close();
		}
	}
}

void BleTransport::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
	if (_disconnectCallback != nullptr)
		_disconnectCallback(this);
	startAdv();
}

void BleTransport::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
	if (pCharacteristic == _ctrlRx)
	{
		NimBLEAttValue value = pCharacteristic->getValue();
		size_t len = value.size();
		if (len == 0) return;
		processor->onTransportDataArrive(millis(), this, value.data(), len);
	}
}

void BleTransport::onStatus(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, int code)
{
	if (pCharacteristic == _ctrlTx)
	{
		if (code == BLE_HS_EDONE)
		{
			_indicateReady = true;
		}
	}
}

