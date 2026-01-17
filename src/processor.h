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

class Processor {
private:
	static Processor *_instance;
	bool _cpuClockHigh = false;
	char _commandBuffer[COMMAND_BUFFER_SIZE+1];
	size_t _commandBufferSize = 0;
	ProcessorState _state = COMMAND_LISTEN;
	WiFiStatus _wifiStatus = WIFI_CLOSE;
	bool _wifiNotifyPending = false;

	char _ssid[SSID_MAX_LENGTH];
	char _password[PASSWORD_MAX_LENGTH];

	byte _fileBuffer[FILE_BUFFER_SIZE];
	char _uploadFileName[32];
	uint _uploadFileSize = 0;
	uint _receiveFileSize = 0;
	uint _receiveChunkSize = 0;
	File _file;
	char _playFileName[80];

	uint32_t _serialDisconnectTime = 0;
	bool _firstConnect = true;

	void stopAudio(const char *soundName=nullptr);
	void stopNotify(int slot);
	void playNotify(int slot);

	const char * getWifiStatusMessage(wl_status_t s);
	static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

	void cmdAbout(uint32_t now);
	void cmdWifi(uint32_t now, const char*ptr);
	void cmdLedOn(uint32_t now, const char*ptr);
	void cmdLedOff(uint32_t now, const char*ptr);
	void cmdPlay(uint32_t now, const char*ptr);
	void cmdStop(uint32_t now);
	void cmdVolume(uint32_t now, const char*ptr);
	void cmdUpload(uint32_t now, const char*ptr);
	void cmdRemove(uint32_t now, const char*ptr);
	void cmdList(uint32_t now);

	size_t commandProcess(uint32_t now, const char *data, size_t size);
	size_t uploadProcess(uint32_t now, const byte *data,  size_t size);
	void parseExecCommand(uint32_t now);

public:

	Processor();
	static const char *getMessageFromCode(int code);
	static void sendNotify(int code, bool hasBody = false);
	static void sendResponse(int code, bool hasBody = false);
	static void sendResponse(int code, bool hasBody, const char *format, ...);
	static void sendBody(const char *format, ...);
	void init();

	const char *WifiSSID();
	const char *WifiPassword();

	void onSerialConnect(uint32_t now);
	void onSerialDisconnect(uint32_t now);
	void onSerialDataArrive(uint32_t now, const byte *data, size_t size);

	void onWifiConnect(uint32_t now);
	void onWifiDisconnect(uint32_t now, uint8_t reason);

	void process(uint32_t now);
};

#endif //SLAPPYBELL_FIRMWARE_PROCESSOR_H