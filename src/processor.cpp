//
// Created by Yasuoki on 2025/12/16.
//

#include <FS.h>
#include <LittleFS.h>
#include <Audio.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "processor.h"
#include "led_sequencer.h"
#include "status_code.h"
#include "utils.h"

char messageBuffer[MESSAGE_BUFFER_SIZE];
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
    _ssid[0] = 0;
    _password[0] = 0;
    _commandBufferSize = 0;
    _instance = this;
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
    default:
        return RC_ERROR;
    }
}

const char * Processor::getWifiStatusMessage(wl_status_t s)
{
    switch(s)
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
    if (Serial)
    {
        const char* statusLine = getMessageFromCode(code);
        Serial.printf(NOTIFY_PREFIX " %s%s\n", statusLine, hasBody ? "+" : "");
    }
}

void Processor::sendResponse(int code, bool hasBody)
{
    if (Serial)
    {
        const char* statusLine = getMessageFromCode(code);
        Serial.printf(RESPONSE_PREFIX " %s%s\n", statusLine, hasBody ? "+" : "");
    }
}

void Processor::sendResponse(int code, bool hasBody, const char* format, ...)
{
    if (Serial)
    {
        if (format != nullptr)
        {
            va_list args;
            va_start(args, format);
            vsnprintf(messageBuffer, MESSAGE_BUFFER_SIZE, format, args);
            va_end(args);
            messageBuffer[MESSAGE_BUFFER_SIZE - 1] = 0;
        }
        else
        {
            messageBuffer[0] = 0;
        }
        Serial.printf(RESPONSE_PREFIX " %s%s%s\n", getMessageFromCode(code), messageBuffer, hasBody ? "+" : "");
    }
}

void Processor::sendBody(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, MESSAGE_BUFFER_SIZE, format, args);
    va_end(args);
    messageBuffer[MESSAGE_BUFFER_SIZE - 1] = 0;
    if (Serial)
    {
        Serial.printf("%s\n", messageBuffer);
    }
}

void Processor::init()
{
    setCpuFrequencyMhz(160);
    if (!LittleFS.begin())
    {
        LittleFS.format();
    }
    pinMode(PIN_SD_MODE, OUTPUT);
    digitalWrite(PIN_SD_MODE, HIGH);
    audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
    audio.setVolume(21); // 0...21

    LedSequencer::init();
}

const char* Processor::WifiSSID()
{
    if (_ssid[0] != 0 && _password[0] != 0)
        return _ssid;
    return nullptr;
}

const char* Processor::WifiPassword()
{
    if (_ssid[0] != 0 && _password[0] != 0)
        return _password;
    return nullptr;
}

void Processor::onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_CONNECTED:
        _instance->onWifiConnect(micros());
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        _instance->onWifiDisconnect(micros(), info.wifi_sta_disconnected.reason);
        break;
    }
}

void Processor::stopAudio(const char* soundName)
{
    if (soundName != nullptr)
    {
        if (strcmp(soundName, _playFileName) == 0)
        {
            audio.stopSong();
            _playFileName[0] = 0;
        }
    }
    else
    {
        audio.stopSong();
        _playFileName[0] = 0;
    }
}

void Processor::cmdAbout(uint32_t now)
{
    sendBody("Yonabe Factory / " PRODUCT_NAME " / " PRODUCT_VERSION);
}

void Processor::cmdWifi(uint32_t now, const char* cmd)
{
    char ssid[SSID_MAX_LENGTH];
    cmd = Utils::skipWs(cmd);
    if (cmd == nullptr || *cmd == '\0')
    {
        const char *st = getWifiStatusMessage(WiFi.status());
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
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    if (_ssid[0] == 0)
    {
        WiFi.onEvent(onWiFiEvent);
    }
    strcpy(_ssid, ssid);
    strcpy(_password, passwd);
    _wifiStatus = WIFI_CONNECTING;
    WiFi.disconnect(true);
    WiFi.setAutoReconnect(true);
    wl_status_t st = WiFi.begin(ssid, passwd);
    if (st != WL_CONNECTED)
        log_d("cmdWifi WiFi.begin(%s,%s)=%d", ssid, passwd, st);
    sendResponse(CD_SUCCESS);
}

void Processor::cmdLedOn(uint32_t now, const char* cmd)
{
    // led-on slot pattern
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
    slot = SLOT_COUNT - slot -1;
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
    int slot;
    cmd = Utils::parseInt(cmd, &slot);
    if (!cmd)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    slot = SLOT_COUNT - slot -1;
    LedSequencer::clear(slot);
    sendResponse(CD_SUCCESS);
}

void Processor::cmdPlay(uint32_t now, const char* ptr)
{
    // play "mp3-file"
    stopAudio();
    _playFileName[0] = 0;
    ptr = Utils::parseString(ptr, _playFileName, sizeof(_playFileName));
    if (!ptr)
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

void Processor::cmdStop(uint32_t now)
{
    stopAudio();
    sendResponse(CD_SUCCESS);
}

void Processor::cmdVolume(uint32_t now, const char* ptr)
{
    // volume 100
    uint volume;
    ptr = Utils::parseUInt(ptr, &volume);
    if (!ptr)
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

void Processor::cmdUpload(uint32_t now, const char* ptr)
{
    // upload "filename" fileSize
    ptr = Utils::parseString(ptr, _uploadFileName, sizeof(_uploadFileName));
    if (!ptr)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    ptr = Utils::parseUInt(ptr, &_uploadFileSize);
    if (!ptr)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    if (_uploadFileName[0] != '/')
    {
        char t[32];
        strcpy(t, _uploadFileName);
        strcpy(&_uploadFileName[1], t);
        _uploadFileName[0] = '/';
    }
    if (strlen(_uploadFileName) == 0 || _uploadFileSize == 0)
    {
        sendResponse(CD_COMMAND_ERROR);
        return;
    }
    stopAudio(_uploadFileName);
    _file = LittleFS.open(_uploadFileName, "w");
    if (!_file)
    {
        sendResponse(CD_FILE_IO_ERROR);
        return;
    }
    _receiveFileSize = 0;
    _receiveChunkSize = 0;
    _state = FILE_UPLOAD;
}

size_t Processor::uploadProcess(uint32_t now, const byte* data, size_t size)
{
    log_d("receive chunk %u/%u", _receiveFileSize, _uploadFileSize);
    size_t remain = _uploadFileSize - _receiveFileSize;
    size_t readSize = size;
    while (remain > 0 && size > 0)
    {
        size_t bufferSpace = sizeof(_fileBuffer) - _receiveChunkSize;
        size_t writeSize = remain < bufferSpace ? remain : bufferSpace;
        if (writeSize > size)
            writeSize = size;

        size_t dataSize = 0;
        byte* dst = &_fileBuffer[_receiveChunkSize];
        while (writeSize > dataSize)
        {
            *dst++ = *data++;
            dataSize++;
        }
        _receiveChunkSize += dataSize;
        _receiveFileSize += dataSize;

        if (_receiveChunkSize == sizeof(_fileBuffer))
        {
            if (_file)
            {
                log_d("flush chunk %u/%u", _receiveFileSize, _uploadFileSize);
                if (_file.write(_fileBuffer, _receiveChunkSize) != _receiveChunkSize)
                {
                    _file.close();
                }
            }
            _receiveChunkSize = 0;
        }
        remain -= dataSize;
        size -= dataSize;
    }
    if (remain == 0)
    {
        if (_receiveChunkSize > 0)
        {
            if (_file)
            {
                if (_file.write(_fileBuffer, _receiveChunkSize) != _receiveChunkSize)
                {
                    _file.close();
                }
            }
        }
        bool success = false;
        if (_file)
        {
            _file.close();
            success = true;
        }
        if (success)
            sendResponse(CD_SUCCESS, false, ", Upload Complete. size=%u", _receiveFileSize);
        else
            sendResponse(CD_FILE_IO_ERROR);
        _state = COMMAND_LISTEN;
    }
    return readSize - size;
}

void Processor::cmdRemove(uint32_t now, const char* ptr)
{
    // R "fileName"
    ptr = Utils::parseString(ptr, _uploadFileName, sizeof(_uploadFileName));
    if (!ptr)
    {
        sendResponse(CD_BAD_COMMAND_FORMAT);
        return;
    }
    if (_uploadFileName[0] != '/')
    {
        char t[32];
        strcpy(t, _uploadFileName);
        strcpy(&_uploadFileName[1], t);
        _uploadFileName[0] = '/';
    }

    if (!LittleFS.exists(_uploadFileName))
    {
        sendResponse(CD_FILE_NOT_FOUND);
        return;
    }
    stopAudio(_uploadFileName);
    if (!LittleFS.remove(_uploadFileName))
    {
        sendResponse(CD_FILE_IO_ERROR);
        return;
    }
    sendResponse(CD_SUCCESS);
}

void Processor::cmdList(uint32_t now)
{
    // L
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    sendResponse(CD_SUCCESS, true);
    sendBody("Storage Usage: %lu/%lu\nFiles:", (ulong)usedBytes, (ulong)totalBytes);

    while (file)
    {
        const char* name = file.name();
        sendBody("%s %lu", name, (ulong)file.size());
        file = root.openNextFile();
    }
    sendBody("");
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
        if (_serialDisconnectTime == 0)
        {
            sendNotify(CD_WIFI_CONNECTED);
            _wifiNotifyPending = false;
        }
        else
        {
            _wifiNotifyPending = true;
        }
    }
}

void Processor::onWifiDisconnect(uint32_t now, uint8_t reason)
{
    if (_wifiStatus != WIFI_DISCONNECTED)
    {
        _wifiStatus = WIFI_DISCONNECTED;
        if (_serialDisconnectTime == 0)
        {
            switch (reason)
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
        else
        {
            _wifiNotifyPending = true;
        }
    }
}

void Processor::onSerialConnect(uint32_t now)
{
    _serialDisconnectTime = 0;
    if (_wifiNotifyPending)
    {
        if (_wifiStatus == WIFI_CONNECTED)
        {
            sendNotify(CD_WIFI_CONNECTED);
        }
        else
        {
            sendNotify(CD_WIFI_DISCONNECTED);
        }
        _wifiNotifyPending = false;
    }
}

void Processor::onSerialDisconnect(uint32_t now)
{
    _serialDisconnectTime = now;
}

void Processor::parseExecCommand(uint32_t now)
{
    log_d("parseExecCommand command=<%s>", _commandBuffer);
    const char* cmp = Utils::skipWs(_commandBuffer);
    if (cmp == nullptr || *cmp == '\0')
    {
        cmdAbout(now);
        return;
    }
    const char* ptr = Utils::strcmp_ptr("wifi", cmp);
    if (ptr)
    {
        cmdWifi(now, ptr);
        return;
    }
    ptr = Utils::strcmp_ptr("led-on", cmp);
    if (ptr && *ptr == ' ')
    {
        cmdLedOn(now, ptr);
        return;
    }
    ptr = Utils::strcmp_ptr("led-off", cmp);
    if (ptr && *ptr == ' ')
    {
        cmdLedOff(now, ptr);
        return;
    }
    ptr = Utils::strcmp_ptr("play", cmp);
    if (ptr && *ptr == ' ')
    {
        cmdPlay(now, ptr);
        return;
    }
    ptr = Utils::strcmp_ptr("stop", cmp);
    if (ptr && *ptr == '\0')
    {
        cmdStop(now);
        return;
    }
    ptr = Utils::strcmp_ptr("volume", cmp);
    if (ptr && *ptr == ' ')
    {
        cmdVolume(now, ptr);
        return;
    }
    ptr = Utils::strcmp_ptr("upload", cmp);
    if (ptr && *ptr == ' ')
    {
        cmdUpload(now, ptr);
        return;
    }
    ptr = Utils::strcmp_ptr("remove", cmp);
    if (ptr && *ptr == ' ')
    {
        cmdRemove(now, ptr);
        return;
    }
    ptr = Utils::strcmp_ptr("list", cmp);
    if (ptr && *ptr == '\0')
    {
        cmdList(now);
        return;
    }
    sendResponse(CD_UNKNOWN_COMMAND);
}

size_t Processor::commandProcess(uint32_t now, const char* data, size_t size)
{
    size_t readSize = 0;
    char* ptr = &_commandBuffer[_commandBufferSize];
    while (readSize < size)
    {
        if (_commandBufferSize + 1 > COMMAND_BUFFER_SIZE)
        {
            _commandBufferSize = 0;
            _state = RESYNC;
            break;
        }
        readSize++;
        if (*data == '\r' || *data == '\n')
        {
            if (*data == '\r' && *(data + 1) == '\n')
            {
                data++;
                readSize++;
            }
            *ptr = 0;
            data++;
            parseExecCommand(now);
            _commandBufferSize = 0;
            if (_state != COMMAND_LISTEN)
                break;
        }
        else
        {
            *ptr++ = *data++;
            _commandBufferSize++;
        }
    }
    return readSize;
}

void Processor::onSerialDataArrive(uint32_t now, const byte* data, size_t size)
{

    if (_firstConnect)
    {
        LedSequencer::clear();
        _firstConnect = false;
    }


    while (size > 0)
    {
        if (_state == RESYNC)
        {
            size--;
            if (*data++ == '\n')
            {
                _state = COMMAND_LISTEN;
            }
        }
        else if (_state == COMMAND_LISTEN)
        {
            size_t readSize = commandProcess(now, (const char*)data, size);
            size -= readSize;
        }
        else if (_state == FILE_UPLOAD)
        {
            size_t readSize = uploadProcess(now, data, size);
            size -= readSize;
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

    LedSequencer::update(now);
    if (_serialDisconnectTime != 0)
    {
        if (now - _serialDisconnectTime > 1000)
        {
            //LedSequencer::clear();
            _serialDisconnectTime = 0;
        }
    }
}
