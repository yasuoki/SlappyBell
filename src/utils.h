//
// Created by Yasuoki on 2025/12/17.
//

#ifndef SLAPPYBELL_FIRMWARE_UTILS_H
#define SLAPPYBELL_FIRMWARE_UTILS_H

#include <Arduino.h>

class Utils {
public:
	static char *strcat_ptr(char *buff, const char *add);
	static char *strprt_ptr(char *buff, const char *fmt, ...);
	static const char *strcmp_ptr(const char *s1, const char *s2);
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