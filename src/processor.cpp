//
// Created by Yasuoki on 2025/12/16.
//

#include <FS.h>
#include <LittleFS.h>
#include <Audio.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "transport.h"
#include "processor.h"
#include "led_sequencer.h"
#include "status_code.h"
#include "utils.h"

Audio audio;

enum CommandId
{
    WiFiConnect,
    LedOn,
    LedOff,
    Play,
    Stop,
    Mp3FileUpload,
    Mp3FileList,
    Mp3FileRemove,
};

Processor* Processor::_instance = nullptr;

Processor::Processor()
{
    _instance = this;
    _cpuClockHigh = true;
    _receiveBuffer[0] = '\0';
    _receiveBufferReadPtr = _receiveBuffer;
    _receiveBufferWritePtr = _receiveBuffer;
    _messageBuffer[0] = '\0';
    _messageBufferRef.overflow = false;
    _messageBufferRef.ptr = nullptr;
    _messageBufferRef.remain = 0;
    _state = COMMAND_LISTEN;

    _wifiStatus = WIFI_CLOSE;
    _wifiDisconnectReason = 0;
    _wifiNotifyPending = false;
    _lastWifiConnectTime = 0;

    _uploadFileSize = 0;
    _receiveFileSize = 0;
    _fileUploadStatus = 0;
    _lastUploadTime = 0;
    _lastAvailable = 0;

    _playFileName[0] = 0;
    _serialDisconnectTime = 0;
    _firstConnect = true;

    _currentTransport = nullptr;
}

const char* Processor::getMessageFromCode(int code)
{
    switch (code)
    {
    case CD_SUCCESS:
        return RC_SUCCESS;
    case CD_COMMAND_ERROR:
        return RC_COMMAND_ERROR;
    case CD_UNKNOWN_COMMAND:
        return RC_UNKNOWN_COMMAND;
    case CD_BAD_COMMAND_FORMAT:
        return RC_BAD_COMMAND_FORMAT;
    case CD_INTEGER_PARSE_ERROR:
        return RC_INTEGER_PARSE_ERROR;
    case CD_STRING_PARSE_ERROR:
        return RC_STRING_PARSE_ERROR;
    case CD_SLOT_ERROR:
        return RC_SLOT_ERROR;
    case CD_BAD_LED_PATTERN:
        return RC_BAD_LED_PATTERN;
    case CD_FILE_NOT_FOUND:
        return RC_FILE_NOT_FOUND;
    case CD_TOO_LONG_COMMAND:
        return RC_TOO_LONG_COMMAND;
    case CD_NEED_PARAMETER:
        return RC_NEED_PARAMETER;
    case CD_BAD_PARAMETER:
        return RC_BAD_PARAMETER;
    case CD_STORAGE_FULL:
        return RC_STORAGE_FULL;
    case CD_FILE_IO_ERROR:
        return RC_FILE_IO_ERROR;
    case CD_NO_WIFI_CONNECTION:
        return RC_NO_WIFI_CONNECTION;
    case CD_WIFI_CONNECT_FAILED:
        return RC_WIFI_CONNECT_FAILED;
    case CD_WIFI_CONNECTED:
        return RC_WIFI_CONNECTED;
    case CD_WIFI_SSID_NOT_FOUND:
        return RC_WIFI_SSID_NOT_FOUND;
    case CD_WIFI_AUTH_FAIL:
        return RC_WIFI_AUTH_FAIL;
    case CD_WIFI_DISCONNECTED:
        return RC_WIFI_DISCONNECTED;
    case CD_TIMEOUT:
        return RC_TIMEOUT;
    case CD_OVERFLOW:
        return RC_OVERFLOW;
    default:
        return RC_ERROR;
    }
}

const char* Processor::getWifiStatusMessage(wl_status_t s)
{
    switch (s)
    {
    case WL_NO_SHIELD:
        return "Disabled";
    case WL_IDLE_STATUS:
        return "Idle";
    case WL_NO_SSID_AVAIL:
        return "NoSSID";
    case WL_SCAN_COMPLETED:
        return "ScanComplete";
    case WL_CONNECTED:
        return "Connected";
    case WL_CONNECT_FAILED:
        return "ConnectFailed";
    case WL_CONNECTION_LOST:
        return "ConnectionLost";
    case WL_DISCONNECTED:
        return "Disconnected";
    default:
        return "Unknown";
    }
}

void Processor::sendNotify(int code, bool hasBody)
{
    const char* statusLine = getMessageFromCode(code);
    Utils::init_buffer(&_messageBufferRef, _messageBuffer, sizeof(_messageBuffer));
    if (hasBody)
    {
        Utils::printf_buffer(&_messageBufferRef, NOTIFY_PREFIX " %s+\n", statusLine);
    }
    else
    {
        if (_currentTransport == nullptr)
            return;
        Utils::printf_buffer(&_messageBufferRef, NOTIFY_PREFIX " %s\n", statusLine);
        _currentTransport->send(_messageBuffer);
    }
}

void Processor::sendResponse(int code, bool hasBody)
{
    const char* statusLine = getMessageFromCode(code);
    Utils::init_buffer(&_messageBufferRef, _messageBuffer, sizeof(_messageBuffer));
    if (hasBody)
    {
        Utils::printf_buffer(&_messageBufferRef, RESPONSE_PREFIX " %s+\n", statusLine);
    }
    else
    {
        if (_currentTransport == nullptr)
            return;
        Utils::printf_buffer(&_messageBufferRef, RESPONSE_PREFIX " %s\n", statusLine);
        _currentTransport->send(_messageBuffer);
    }
}

void Processor::sendResponse(int code, bool hasBody, const char* format, ...)
{
    Utils::init_buffer(&_messageBufferRef, _messageBuffer, sizeof(_messageBuffer));
    Utils::printf_buffer(&_messageBufferRef, RESPONSE_PREFIX " %s", getMessageFromCode(code));
    if (format != nullptr)
    {
        va_list args;
        va_start(args, format);
        Utils::printf_buffer(&_messageBufferRef, format, args);
        va_end(args);
    }
    if (hasBody)
    {
        Utils::strcat_buffer(&_messageBufferRef, "+\n");
    }
    else
    {
        if (_currentTransport == nullptr)
            return;
        Utils::strcat_buffer(&_messageBufferRef, "\n");
        _currentTransport->send(_messageBuffer);
    }
}

void Processor::sendBody(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Utils::printf_buffer(&_messageBufferRef, format, args);
    va_end(args);
    Utils::strcat_buffer(&_messageBufferRef, "\n");
}

void Processor::sendEnd()
{
    if (_currentTransport == nullptr)
        return;
    Utils::strcat_buffer(&_messageBufferRef, "\n");
    _currentTransport->send(_messageBuffer);
}

void Processor::flushSendBuffer()
{
    if (_currentTransport != nullptr)
    {
        _currentTransport->send(_messageBuffer);
    }
    Utils::init_buffer(&_messageBufferRef, _messageBuffer, sizeof(_messageBuffer));
}

void Processor::sendMessage(const char* message)
{
    if (_currentTransport == nullptr)
        return;
    _currentTransport->send(message);
}

void Processor::init()
{
    setCpuFrequencyMhz(160);
    _cpuClockHigh = false;
    if (!LittleFS.begin())
    {
        LittleFS.format();
    }
    pinMode(PIN_SD_MODE, OUTPUT);
    digitalWrite(PIN_SD_MODE, HIGH);
    audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
    audio.setVolume(21); // 0...21
    WiFi.onEvent(onWiFiEvent);
    LedSequencer::init();
}

void Processor::onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_CONNECTED:
        _instance->onWifiConnect(millis());
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        _instance->onWifiDisconnect(millis(), info.wifi_sta_disconnected.reason);
        break;
    }
}

void Processor::wifiConnect(const char* ssid, const char* passwd)
{
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, passwd);
    _wifiStatus = WIFI_CONNECTING;
    _lastWifiConnectTime = millis();
}

void Processor::wifiDisconnect()
{
    WiFi.setAutoReconnect(false);
    for (int i = 0; i < 5; i++)
    {
        while (WiFi.status() == WL_CONNECTED)
        {
            WiFi.disconnect();
            vTaskDelay(100);
        }
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        sendResponse(CD_ERROR);
        return;
    }
    _wifiStatus = WIFI_CLOSE;
    WiFi.setAutoReconnect(false);
}

void Processor::stopAudio(const char* soundName)
{
    if (soundName != nullptr && strcmp(soundName, _playFileName) != 0)
        return;
    audio.stopSong();
    _playFileName[0] = 0;
}

void Processor::cmdAbout(uint32_t now)
{
    sendMessage("Yonabe Factory / " PRODUCT_NAME " / " PRODUCT_VERSION "\n");
}

void Processor::cmdPing(uint32_t now, const char* cmd)
{
    if (*cmd != '\0')
    {
        sendResponse(CD_BAD_PARAMETER);
        return;
    }
    sendResponse(CD_SUCCESS);
}

void Processor::cmdBye(uint32_t now, const char* ptr)
{
    if (*ptr != '\0')
    {
        sendResponse(CD_BAD_PARAMETER);
    } else
    {
        sendResponse(CD_SUCCESS);
    }
    if (_currentTransport != nullptr)
    {
        _currentTransport->flush();
        _currentTransport->close();
        onTransportDisconnect(now, _currentTransport);
    }
    else
    {
        cancelProcess();
    }
}

void Processor::cmdWifi(uint32_t now, const char* cmd)
{
    char ssid[SSID_MAX_LENGTH];
    cmd = Utils::skipWs(cmd);
    if (cmd == nullptr || *cmd == '\0')
    {
        const char* st = getWifiStatusMessage(WiFi.status());
        sendResponse(CD_SUCCESS, false, ", %s", st);
        return;
    }
    cmd = Utils::parseString(cmd, ssid, sizeof(ssid));
    if (cmd == nullptr)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    char passwd[PASSWORD_MAX_LENGTH];
    cmd = Utils::parseString(cmd, passwd, sizeof(passwd));
    if (cmd == nullptr)
    {
        if (strcmp(ssid, "off") == 0)
        {
            wifiDisconnect();
            sendResponse(CD_SUCCESS);
            return;
        }
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    if (_wifiStatus == WIFI_CONNECTED)
    {
        wifiDisconnect();
    }
    wifiConnect(ssid, passwd);
    sendResponse(CD_SUCCESS);
}

void Processor::cmdLedOn(uint32_t now, const char* cmd)
{
    // led-on slot pattern
    if (*cmd != ' ')
    {
        sendResponse(CD_NEED_PARAMETER);
        return;
    }
    int slot;
    cmd = Utils::parseInt(cmd, &slot);
    if (!cmd)
    {
        sendResponse(CD_INTEGER_PARSE_ERROR);
        return;
    }
    if (slot < 0 || SLOT_COUNT <= slot)
    {
        sendResponse(CD_SLOT_ERROR);
        return;
    }
    slot = SLOT_COUNT - slot - 1;
    if (!LedSequencer::parse(slot, cmd))
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    sendResponse(CD_SUCCESS);
}

void Processor::cmdLedOff(uint32_t now, const char* cmd)
{
    // led-off slot
    if (*cmd != ' ')
    {
        sendResponse(CD_NEED_PARAMETER);
        return;
    }
    int slot;
    cmd = Utils::parseInt(cmd, &slot);
    if (!cmd)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    slot = SLOT_COUNT - slot - 1;
    LedSequencer::clear(slot);
    sendResponse(CD_SUCCESS);
}

void Processor::cmdPlay(uint32_t now, const char* cmd)
{
    // play "mp3-file"
    if (*cmd != ' ')
    {
        sendResponse(CD_NEED_PARAMETER);
        return;
    }
    stopAudio();
    _playFileName[0] = 0;

    cmd = Utils::parseString(cmd, _playFileName, sizeof(_playFileName));
    if (!cmd)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    if (_playFileName[0] != 0)
    {
        if (Utils::strcmp_ptr("http://", _playFileName))
        {
            if (_wifiStatus != WIFI_CONNECTED)
            {
                sendResponse(CD_NO_WIFI_CONNECTION);
                return;
            }
            setCpuFrequencyMhz(240);
            _cpuClockHigh = true;
            if (!audio.connecttohost(_playFileName))
            {
                sendResponse(CD_FILE_IO_ERROR);
                return;
            }
        }
        else
        {
            if (_playFileName[0] != '/')
            {
                char t[32];
                strcpy(t, _playFileName);
                strcpy(&_playFileName[1], t);
                _playFileName[0] = '/';
            }
            if (!LittleFS.exists(_playFileName))
            {
                sendResponse(CD_FILE_NOT_FOUND);
                return;
            }
            setCpuFrequencyMhz(240);
            _cpuClockHigh = true;
            if (!audio.connecttoFS(LittleFS, _playFileName))
            {
                sendResponse(CD_FILE_IO_ERROR);
                return;
            }
        }
    }
    sendResponse(CD_SUCCESS);
}

void Processor::cmdStop(uint32_t now, const char* cmd)
{
    if (*cmd != '\0')
    {
        sendResponse(CD_BAD_PARAMETER);
        return;
    }
    stopAudio();
    sendResponse(CD_SUCCESS);
}

void Processor::cmdVolume(uint32_t now, const char* cmd)
{
    // volume 100
    if (*cmd != ' ')
    {
        sendResponse(CD_NEED_PARAMETER);
        return;
    }
    uint volume;
    cmd = Utils::parseUInt(cmd, &volume);
    if (!cmd)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    if (volume < 0 || volume > 100)
    {
        sendResponse(CD_COMMAND_ERROR);
        return;
    }
    uint8_t v = (uint8_t)(21.0f * (float)volume / 100.0f);;
    if (v == 0 && volume > 0)
        v = 1;
    audio.setVolume(v);
    sendResponse(CD_SUCCESS);
}

void Processor::cmdUpload(uint32_t now, const char* cmd)
{
    // upload "filename" fileSize
    char fileName[32];
    if (*cmd != ' ')
    {
        sendResponse(CD_NEED_PARAMETER);
        return;
    }
    cmd = Utils::parseString(cmd, fileName, sizeof(fileName));
    if (!cmd)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    cmd = Utils::parseUInt(cmd, &_uploadFileSize);
    if (!cmd)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    if (fileName[0] != '/')
    {
        char t[32];
        strcpy(t, fileName);
        strcpy(&fileName[1], t);
        fileName[0] = '/';
    }
    if (strlen(fileName) == 0 || _uploadFileSize == 0)
    {
        sendResponse(CD_COMMAND_ERROR);
        return;
    }
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    if (freeBytes < _uploadFileSize)
    {
        sendResponse(CD_STORAGE_FULL);
        return;
    }

    stopAudio(fileName);
    _fileUploadStatus = CD_SUCCESS;
    _uploadFile = LittleFS.open(fileName, "w");
    if (!_uploadFile)
    {
        _fileUploadStatus = CD_FILE_IO_ERROR;
        sendResponse(CD_FILE_IO_ERROR);
        return;
    }

    _receiveFileSize = 0;
    _state = FILE_UPLOAD;
    _lastUploadTime = 0;
    _lastAvailable = 0;
    sendResponse(CD_SUCCESS, false, ", Upload Start. size=%u", _uploadFileSize);
}

void Processor::cmdRemove(uint32_t now, const char* cmd)
{
    // remove "fileName"
    char fileName[32];
    if (*cmd != ' ')
    {
        sendResponse(CD_NEED_PARAMETER);
        return;
    }
    cmd = Utils::parseString(cmd, fileName, sizeof(fileName));
    if (!cmd)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    if (fileName[0] != '/')
    {
        char t[32];
        strcpy(t, fileName);
        strcpy(&fileName[1], t);
        fileName[0] = '/';
    }

    if (!LittleFS.exists(fileName))
    {
        sendResponse(CD_FILE_NOT_FOUND);
        return;
    }
    stopAudio(fileName);
    if (!LittleFS.remove(fileName))
    {
        sendResponse(CD_FILE_IO_ERROR);
        return;
    }
    sendResponse(CD_SUCCESS);
}

void Processor::cmdList(uint32_t now, const char* cmd)
{
    // List
    if (*cmd != '\0')
    {
        sendResponse(CD_BAD_PARAMETER);
        return;
    }
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    sendResponse(CD_SUCCESS, true);
    sendBody("Storage Usage: %lu/%lu\nFiles:", (ulong)usedBytes, (ulong)totalBytes);
    while (file)
    {
        const char* name = file.name();
        if (strlen(name) + 10 > _messageBufferRef.remain)
        {
            flushSendBuffer();
        }
        sendBody("%s %lu", name, (ulong)file.size());
        file = root.openNextFile();
    }
    sendEnd();
}

void Processor::onWifiConnect(uint32_t now)
{
    if (_wifiStatus != WIFI_CONNECTED)
    {
        if (_wifiStatus == WIFI_CONNECTING)
        {
            esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        }
        _wifiStatus = WIFI_CONNECTED;
        _wifiNotifyPending = true;
    }
}

void Processor::onWifiDisconnect(uint32_t now, uint8_t reason)
{
    if (_wifiStatus != WIFI_DISCONNECTED)
    {
        _wifiStatus = WIFI_DISCONNECTED;
        _wifiDisconnectReason = reason;
        _wifiNotifyPending = true;
    }
}

bool Processor::onTransportConnect(uint32_t now, Transport* transport)
{
    _serialDisconnectTime = 0;
    if (_currentTransport == transport)
        return true;
    if (_currentTransport != nullptr)
    {
        if (_currentTransport->getType() == TransportType::SERIAL_TRANSPORT)
        {
            return false;
        }
        cancelProcess();
        _currentTransport->close();
        _currentTransport = nullptr;
    }

    _currentTransport = transport;
    if (_firstConnect)
    {
        LedSequencer::clear();
        _firstConnect = false;
    }
    return true;
}

void Processor::onTransportDisconnect(uint32_t now, Transport* transport)
{
    if (_currentTransport == transport)
    {
        _currentTransport = nullptr;
        cancelProcess();
    }
}

void Processor::onSerialConnect(uint32_t now, Transport* transport)
{
    if (_serialDisconnectTime != 0)
    {
        _serialDisconnectTime = 0;
        return;
    }
    onTransportConnect(now, transport);
}

void Processor::onSerialDisconnect(uint32_t now, Transport* transport)
{
    _serialDisconnectTime = now;
}

void Processor::commandProcess(uint32_t now, const char* line)
{
    const char* cmp = Utils::skipWs(line);
    if (cmp == nullptr || *cmp == '\0')
    {
        cmdAbout(now);
        return;
    }
    const char* ptr = Utils::is_symbol_ptr("ping", cmp);
    if (ptr)
    {
        cmdPing(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("bye", cmp);
    if (ptr)
    {
        cmdBye(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("wifi", cmp);
    if (ptr)
    {
        cmdWifi(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("led-on", cmp);
    if (ptr)
    {
        cmdLedOn(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("led-off", cmp);
    if (ptr)
    {
        cmdLedOff(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("play", cmp);
    if (ptr)
    {
        cmdPlay(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("stop", cmp);
    if (ptr)
    {
        cmdStop(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("volume", cmp);
    if (ptr)
    {
        cmdVolume(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("upload", cmp);
    if (ptr)
    {
        cmdUpload(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("remove", cmp);
    if (ptr)
    {
        cmdRemove(now, ptr);
        return;
    }
    ptr = Utils::is_symbol_ptr("list", cmp);
    if (ptr)
    {
        cmdList(now, ptr);
        return;
    }
    sendResponse(CD_UNKNOWN_COMMAND);
}

size_t Processor::uploadProcess(uint32_t now)
{
    size_t remain = _uploadFileSize - _receiveFileSize;
    size_t available = receiveBufferAvailable();
    size_t readSize;
    if (available == 0)
        return 0;

    if (_lastAvailable == available)
    {
        return 0;
    }
    _lastAvailable = available;
    if (available < remain)
    {
        if (available < RECEIVE_BUFFER_SIZE)
        {
            return 0;
        }
        readSize = RECEIVE_BUFFER_SIZE;
    }
    else
    {
        readSize = remain;
    }
    _receiveFileSize += readSize;
    const byte* buffer = readReceiveBuffer(readSize);
    if (buffer == nullptr)
    {
        _uploadFile.close();
        _fileUploadStatus = CD_ERROR;
        return 0;
    }
    if (_uploadFile && _fileUploadStatus == CD_SUCCESS)
    {
        if (_uploadFile.write(buffer, readSize) != readSize)
        {
            _uploadFile.close();
            _fileUploadStatus = CD_FILE_IO_ERROR;
        }
    }
    if (_receiveFileSize == _uploadFileSize)
    {
        bool success = false;
        if (_uploadFile && _fileUploadStatus == CD_SUCCESS)
        {
            _uploadFile.close();
            success = true;
        }
        if (success)
            sendResponse(CD_SUCCESS, false, ", Upload Complete. size=%u", _receiveFileSize);
        else
            sendResponse(_fileUploadStatus);
        _state = COMMAND_LISTEN;
        _lastUploadTime = 0;
    }
    return readSize;
}

size_t Processor::writeReceiveBuffer(const byte* data, size_t size)
{
    byte* ptr = _receiveBufferWritePtr;
    size_t remain = sizeof(_receiveBuffer) - (_receiveBufferWritePtr - _receiveBufferReadPtr);
    size_t writeSize = remain < size ? remain : size;
    memcpy(ptr, data, writeSize);
    _receiveBufferWritePtr += writeSize;
    if (writeSize < size)
    {
        sendNotify(CD_OVERFLOW);
    }
    return writeSize;
}

const char* Processor::readLineReceiveBuffer()
{
    byte* line = _receiveBufferReadPtr;
    byte* ptr = _receiveBufferReadPtr;
    while (ptr != _receiveBufferWritePtr)
    {
        char c = *(char*)ptr;
        if (c == '\r' || c == '\n')
        {
            *ptr++ = 0;
            if (c == '\r' && ptr != _receiveBufferWritePtr && *(char*)ptr == '\n')
            {
                ptr++;
            }
            if (ptr == _receiveBufferWritePtr)
            {
                _receiveBufferReadPtr = _receiveBuffer;
                _receiveBufferWritePtr = _receiveBuffer;
            }
            else
            {
                _receiveBufferReadPtr = ptr;
            }
            return (char*)line;
        }
        ptr++;
    }
    return nullptr;
}

const byte* Processor::readReceiveBuffer(size_t size)
{
    size_t remain = _receiveBufferWritePtr - _receiveBufferReadPtr;
    if (remain < size)
        return nullptr;
    const byte* ptr = _receiveBufferReadPtr;
    _receiveBufferReadPtr += size;

    if (_receiveBufferReadPtr == _receiveBufferWritePtr)
    {
        _receiveBufferReadPtr = _receiveBuffer;
        _receiveBufferWritePtr = _receiveBuffer;
    }
    else if (_receiveBufferReadPtr >= _receiveBuffer + RECEIVE_BUFFER_SIZE)
    {
        remain = _receiveBufferWritePtr - _receiveBufferReadPtr;
        memcpy(_receiveBuffer, _receiveBufferReadPtr, remain);
        _receiveBufferWritePtr = _receiveBuffer + remain;
        _receiveBufferReadPtr = _receiveBuffer;
    }
    return ptr;
}

bool Processor::receiveBufferIsEmpty() const
{
    return _receiveBufferReadPtr == _receiveBufferWritePtr;
}

size_t Processor::receiveBufferAvailable() const
{
    return _receiveBufferWritePtr - _receiveBufferReadPtr;
}

void Processor::cancelProcess()
{
    _state = COMMAND_LISTEN;
    if (_uploadFile)
    {
        _uploadFile.close();
    }
    _lastUploadTime = 0;
    _receiveBufferWritePtr = _receiveBuffer;
    _receiveBufferReadPtr = _receiveBuffer;
    _receiveFileSize = 0;
}

void Processor::onTransportDataArrive(uint32_t now, Transport* transport, const byte* data, size_t size)
{
    _currentTransport = transport;
    if (_state == FILE_UPLOAD)
    {
        _lastUploadTime = now;
    }
    writeReceiveBuffer(data, size);
}

void Processor::dataProcess(uint32_t now)
{
    while (!receiveBufferIsEmpty())
    {
        if (_state == COMMAND_LISTEN || _state == RESYNC)
        {
            const char* line = readLineReceiveBuffer();
            if (line == nullptr)
            {
                break;
            }
            commandProcess(now, line);
        }
        else if (_state == FILE_UPLOAD)
        {
            uploadProcess(now);
            break;
        }
    }
}

void Processor::timeProcess(uint32_t now)
{
    if (_wifiStatus == WIFI_CONNECTING)
    {
        if (now - _lastWifiConnectTime > 1000)
        {
            WiFi.disconnect();
            WiFi.reconnect();
            _lastWifiConnectTime = now;
        }
    }
    if (_wifiNotifyPending)
    {
        if(_state == COMMAND_LISTEN && _currentTransport != nullptr && _serialDisconnectTime == 0)
        {
            if (_wifiStatus == WIFI_CONNECTED)
            {
                sendNotify(CD_WIFI_CONNECTED);
                _wifiNotifyPending = false;
            }
            else if (_wifiStatus == WIFI_DISCONNECTED)
            {
                switch (_wifiDisconnectReason)
                {
                case WIFI_REASON_AUTH_EXPIRE:
                case WIFI_REASON_ASSOC_EXPIRE:
                case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                    sendNotify(CD_WIFI_AUTH_FAIL);
                    break;
                case WIFI_REASON_NO_AP_FOUND:
                    sendNotify(CD_WIFI_SSID_NOT_FOUND);
                    break;
                default:
                    sendNotify(CD_WIFI_DISCONNECTED);
                }
                _wifiNotifyPending = false;
            }
        }
    }
}

void Processor::process(uint32_t now)
{
    audio.loop();
    if (!audio.isRunning() && _cpuClockHigh)
    {
        setCpuFrequencyMhz(160);
        _cpuClockHigh = false;
    }
    if (_state == FILE_UPLOAD)
    {
        if (_lastUploadTime != 0 && now - _lastUploadTime > DATA_CHUNK_TIMEOUT)
        {
            cancelProcess();
            sendNotify(CD_TIMEOUT);
            return;
        }
    }
    if (!receiveBufferIsEmpty())
    {
        dataProcess(now);
    }
    else
    {
        timeProcess(now);
    }

    LedSequencer::update(now);
    if (_serialDisconnectTime != 0)
    {
        if (_currentTransport != nullptr && _currentTransport->getType() == SERIAL_TRANSPORT)
        {
            if (now - _serialDisconnectTime > 1000)
            {
                _serialDisconnectTime = 0;
                onTransportDisconnect(now, _currentTransport);
            }
        }
    }
}
