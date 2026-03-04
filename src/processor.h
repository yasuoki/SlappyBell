//
// Created by Yasuoki on 2025/12/16.
//

#ifndef SLAPPYBELL_FIRMWARE_PROCESSOR_H
#define SLAPPYBELL_FIRMWARE_PROCESSOR_H

#include <Arduino.h>
#include <WiFi.h>
#include <Audio.h>
#include "config.h"
#include "led_sequencer.h"
#include "utils.h"

enum ProcessorState {
	COMMAND_LISTEN,
	FILE_UPLOAD,
	RESYNC,
};

enum WiFiStatus
{
	WIFI_CLOSE,
	WIFI_CONNECTING,
	WIFI_CONNECTED,
	WIFI_DISCONNECTED
};
class Transport;
class Processor {
private:
	static Processor *_instance;
	bool _cpuClockHigh;
	byte _receiveBuffer[RECEIVE_BUFFER_SIZE+RECEIVE_BUFFER_OVERFLOW_SIZE]{};
	byte* _receiveBufferReadPtr = nullptr;
	byte* _receiveBufferWritePtr = nullptr;
	char _messageBuffer[MESSAGE_BUFFER_SIZE]{};
	STR_BUFFER _messageBufferRef{};
	ProcessorState _state;

	WiFiStatus _wifiStatus;
	uint8_t _wifiDisconnectReason;
	bool _wifiNotifyPending;
	uint32_t _lastWifiConnectTime;

	uint _uploadFileSize;
	uint _receiveFileSize;
	File _uploadFile;
	int _fileUploadStatus;
	uint32_t _lastUploadTime;
	size_t _lastAvailable;

	char _playFileName[80]{};

	uint32_t _serialDisconnectTime;
	bool _firstConnect;

	Transport * _currentTransport = nullptr;

	void stopAudio(const char *soundName=nullptr);

	static const char * getWifiStatusMessage(wl_status_t s);
	static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
	void wifiDisconnect();
	void wifiConnect(const char* ssid, const char *passwd);

	void cmdAbout(uint32_t now);
	void cmdPing(uint32_t now, const char*ptr);
	void cmdBye(uint32_t now, const char*ptr);
	void cmdWifi(uint32_t now, const char*ptr);
	void cmdLedOn(uint32_t now, const char*ptr);
	void cmdLedOff(uint32_t now, const char*ptr);
	void cmdPlay(uint32_t now, const char*cmd);
	void cmdStop(uint32_t now, const char *cmd);
	void cmdVolume(uint32_t now, const char*cmd);
	void cmdUpload(uint32_t now, const char*cmd);
	void cmdRemove(uint32_t now, const char*cmd);
	void cmdList(uint32_t now, const char *cmd);

	size_t writeReceiveBuffer(const byte* data, size_t size);
	const char *readLineReceiveBuffer();
	const byte *readReceiveBuffer(size_t size);
	bool receiveBufferIsEmpty() const;
	size_t receiveBufferAvailable() const;

	void cancelProcess();
	void commandProcess(uint32_t now, const char *line);;
	size_t uploadProcess(uint32_t now);
	void dataProcess(uint32_t now);
	void timeProcess(uint32_t now);

	void sendNotify(int code, bool hasBody = false);
	void sendResponse(int code, bool hasBody = false);
	void sendResponse(int code, bool hasBody, const char *format, ...);
	void sendBody(const char *format, ...);
	void sendEnd();
	void flushSendBuffer();
	void sendMessage(const char *message);

public:

	Processor();
	static const char *getMessageFromCode(int code);
	void init();

	bool onTransportConnect(uint32_t now, Transport *transport);
	void onTransportDisconnect(uint32_t now, Transport *transport);
	void onSerialConnect(uint32_t now, Transport *transport);
	void onSerialDisconnect(uint32_t now, Transport *transport);
	void onTransportDataArrive(uint32_t now, Transport *transport, const byte *data, size_t size);

	void onWifiConnect(uint32_t now);
	void onWifiDisconnect(uint32_t now, uint8_t reason);

	void process(uint32_t now);
};

#endif //SLAPPYBELL_FIRMWARE_PROCESSOR_H