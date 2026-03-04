//
// Created by Yasuoki on 2025/12/17.
//

#ifndef SLAPPYBELL_FIRMWARE_UTILS_H
#define SLAPPYBELL_FIRMWARE_UTILS_H

#include <Arduino.h>

typedef struct _STR_BUFFER
{
	char*  ptr;        // 現在位置
	size_t remain;     // 残り容量（null含む）
	bool   overflow;   // 切り詰め発生
} STR_BUFFER;

class Utils {
public:
	static void init_buffer(STR_BUFFER *buffer, char *buff, size_t len);
	static void empty_buffer(STR_BUFFER *buffer);
	static bool is_empty_buffer(STR_BUFFER *buffer);
	static char *strcat_ptr(char *buff, size_t len, const char *add);
	static char *strprt_ptr(char *buff, size_t len, const char *fmt, ...);
	static char *strprt_ptr(char *buff, size_t len, const char *fmt, va_list args);
	static size_t strcat_buffer(STR_BUFFER *buff, const char *add);
	static size_t printf_buffer(STR_BUFFER *buffer, const char *fmt, ...);
	static size_t printf_buffer(STR_BUFFER *buffer, const char *fmt, va_list args);
	static const char *strcmp_ptr(const char *s1, const char *s2);
	static const char *is_symbol_ptr(const char *s1, const char *s2);
	static const char * skipWs(const char *s);
	static const char * nextChar(const char *s);
	static const char * parseInt(const char*s, int *val);
	static const char * parseUInt(const char*s, uint *val);
	static const char * parseString(const char*s, char *val, size_t maxLen);
	static bool isHex(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
	static byte hexToBin(char c);
	static const char * parseHex4(const char *p, byte *val);
	static const char * parseHexByte(const char *p, byte *val);
	static const char * parseHex(const char*p, uint32_t *val);
	static uint16_t crc16(const uint8_t* buff, size_t size);;
};


#endif //SLAPPYBELL_FIRMWARE_UTILS_H