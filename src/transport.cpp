//
// Created by yasuoki on 2026/02/06.
//

#include "transport.h"

size_t Transport::printf(const char *format, ...) {
	char loc_buf[64];
	char * temp = loc_buf;
	va_list arg;
	va_list copy;
	va_start(arg, format);
	va_copy(copy, arg);
	int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
	va_end(copy);
	if(len < 0) {
		va_end(arg);
		return 0;
	}
	if(len >= (int)sizeof(loc_buf)){  // comparation of same sign type for the compiler
		temp = (char*) malloc(len+1);
		if(temp == NULL) {
			va_end(arg);
			return 0;
		}
		len = vsnprintf(temp, len+1, format, arg);
	}
	va_end(arg);
	len = send((uint8_t*)temp, len);
	if(temp != loc_buf){
		free(temp);
	}
	return len;
}

size_t Transport::send(const char *text) {
	return send((uint8_t*)text, strlen(text));
}

