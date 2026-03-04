//
// Created by Yasuoki on 2025/12/17.
//

#include <Arduino.h>
#include "utils.h"
#include "config.h"

void Utils::init_buffer(STR_BUFFER* buffer, char* mem, size_t len)
{
    buffer->ptr      = mem;
    buffer->remain   = len;
    buffer->overflow = false;

    if (len > 0)
    {
        mem[0] = '\0';
    }
}

void Utils::empty_buffer(STR_BUFFER* buffer)
{
    buffer->ptr      = nullptr;
    buffer->remain   = 0;
    buffer->overflow = false;
}

bool Utils::is_empty_buffer(STR_BUFFER* buffer)
{
    return buffer->ptr == nullptr;
}

char *Utils::strcat_ptr(char *buff, size_t len, const char *add) {
    char *p = buff;
    size_t n = 0;
    while(*add && n < len-1)
        *p++ = *add++;
    *p = 0;
    return p;
}

char *Utils::strprt_ptr(char *buff, size_t len, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int plen = vsnprintf(buff, len, fmt, args);
    va_end(args);
    buff[len-1] = 0;
    if (plen > len)
        return buff+len;
    else
        return buff+plen;
}

char *Utils::strprt_ptr(char *buff, size_t len, const char *fmt, va_list args)
{
    int plen = vsnprintf(buff, len, fmt, args);
    buff[len-1] = 0;
    if (plen > len)
        return buff+len;
    else
        return buff+plen;
}

size_t Utils::strcat_buffer(STR_BUFFER* b, const char* add)
{
    if (b->remain <= 1)
    {
        b->overflow = true;
        return 0;
    }

    size_t len = strlen(add);
    size_t copy = (len < b->remain - 1) ? len : b->remain - 1;

    memcpy(b->ptr, add, copy);

    b->ptr    += copy;
    b->remain -= copy;

    *b->ptr = '\0';

    if (copy < len)
        b->overflow = true;

    return copy;
}

size_t Utils::printf_buffer(STR_BUFFER* b, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t r = printf_buffer(b, fmt, args);
    va_end(args);
    return r;
}

size_t Utils::printf_buffer(STR_BUFFER* b, const char* fmt, va_list args)
{
    if (b->remain <= 1)
    {
        b->overflow = true;
        return 0;
    }

    int ret = vsnprintf(b->ptr, b->remain, fmt, args);

    if (ret < 0)
        return 0;

    size_t written;

    if ((size_t)ret >= b->remain)
    {
        written = b->remain - 1;
        b->overflow = true;
    }
    else
    {
        written = ret;
    }

    b->ptr    += written;
    b->remain -= written;

    return written;
}

const char *Utils::strcmp_ptr(const char *s1, const char *s2) {

    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    if (*s1 == 0)
        return s2;
    return nullptr;
}

const char* Utils::is_symbol_ptr(const char* s1, const char* s2)
{
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    if (*s1 == 0)
    {
        if (*s2 == ' ' || *s2 == '\0')
            return s2;
    }
    return nullptr;
}

const char * Utils::skipWs(const char *s) {
    while (*s != 0 && *s == ' ')
        s++;
    if (*s == 0)
        return nullptr;
    return s;
}

const char *Utils::nextChar(const char *s) {
    if (*s && *(s + 1))
        return s + 1;
    return nullptr;
}

const char *Utils::parseInt(const char *s, int *val) {
    int n = 0;
    int f = 1;

    s = skipWs(s);
    if (s == nullptr)
        return nullptr;

    const char *p = s;
    if (*p == '-') {
        f = -1;
        p++;
    }
    while ('0' <= *p && *p <= '9') {
        n = n * 10 + (*p++ - '0');
    }
    if (s == p)
        return nullptr;
    *val = n * f;
    return p;
}

const char *Utils::parseUInt(const char *s, uint *val) {
    uint n = 0;

    s = skipWs(s);
    if (s == nullptr)
        return nullptr;
    const char *p = s;
    while ('0' <= *p && *p <= '9') {
        n = n * 10 + (*p++ - '0');
    }
    if (s == p)
        return nullptr;
    *val = n;
    return p;
}

const char *Utils::parseString(const char*s, char *val, size_t maxLen) {
    s = skipWs(s);
    if (!s) {
        return nullptr;
    }
    char endChar = ' ';
    if (*s == '"') {
        s++;
        endChar = '"';
    }
    while (*s && *s != endChar && maxLen > 0) {
        *val++ = *s++;
        maxLen--;
    }
    if ((endChar == '"' && *s != endChar) || (endChar == ' ' && (*s != ' ' && *s != '\0')))
        return nullptr;
    *val = 0;
    return endChar == '"' ? s + 1 : s;
}

byte Utils::hexToBin(char c) {
    byte d = 0;
    if ('0' <= c && c <= '9') {
        d = c - '0';
    } else if ('a' <= c && c <= 'f') {
        d = c - 'a' + 10;
    } else if ('A' <= c && c <= 'F') {
        d = c - 'A' + 10;
    } else {
        return 0;
    }
    return d;
}

const char *Utils::parseHexByte(const char *p, byte *val) {
    byte d = 0;
    int n;
    for (n = 0; n < 2; n++) {
        if (!isHex(*p))
            break;
        d = (d << 4) + hexToBin(*p++);
    }
    if (n == 0)
        return nullptr;
    *val = d;
    return p;
}

const char *Utils::parseHex(const char *p, uint32_t *val) {
    uint32_t d = 0;
    int n;
    for (n = 0; n < 8; n++) {
        if (!isHex(*p))
            break;
        d = (d << 4) + hexToBin(*p++);
    }
    if (n == 0)
        return nullptr;
    *val = d;
    return p;
}

uint16_t Utils::crc16(const uint8_t* buff, size_t size)
{
    uint8_t* data = (uint8_t*)buff;
    uint16_t result = 0xFFFF;

    for (size_t i = 0; i < size; ++i)
    {
        result ^= data[i];
        for (size_t j = 0; j < 8; ++j)
        {
            if (result & 0x01) result = (result >> 1) ^ 0xA001;
            else result >>= 1;
        }
    }
    return result;
}
