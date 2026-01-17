//
// Created by Yasuoki on 2025/12/17.
//

#ifndef SLAPPYBELL_FIRMWARE_STATUS_CODE_H
#define SLAPPYBELL_FIRMWARE_STATUS_CODE_H

#define CD_SUCCESS					 0
#define RC_SUCCESS					"00 OK"

#define CD_COMMAND_ERROR			 10
#define RC_COMMAND_ERROR			"10 Command error"
#define CD_UNKNOWN_COMMAND			 11
#define RC_UNKNOWN_COMMAND			"11 Unknown command"
#define CD_BAD_COMMAND_FORMAT		 12
#define RC_BAD_COMMAND_FORMAT		"12 Bad command format"
#define CD_INTEGER_PARSE_ERROR		 13
#define RC_INTEGER_PARSE_ERROR		"13 Integer parse error"
#define CD_STRING_PARSE_ERROR		 14
#define RC_STRING_PARSE_ERROR		"14 Integer parse error"
#define CD_SLOT_ERROR				 20
#define RC_SLOT_ERROR				"20 Slot error"
#define CD_BAD_LED_PATTERN			 21
#define RC_BAD_LED_PATTERN			"21 Bad LED pattern"
#define CD_FILE_NOT_FOUND			 22
#define RC_FILE_NOT_FOUND			"22 File not found"
#define CD_TOO_LONG_COMMAND			 23
#define RC_TOO_LONG_COMMAND			"23 Too long command"
#define CD_STORAGE_FULL				 30
#define RC_STORAGE_FULL				"30 Storage full"
#define CD_FILE_IO_ERROR			 31
#define RC_FILE_IO_ERROR			"31 File IO error"
#define CD_NO_WIFI_CONNECTION		 32
#define RC_NO_WIFI_CONNECTION		"32 No Wi-Fi connection"
#define CD_WIFI_CONNECT_FAILED		 33
#define RC_WIFI_CONNECT_FAILED		"33 Wi-Fi connect failed"
#define CD_WIFI_CONNECTED			 50
#define RC_WIFI_CONNECTED			"50 Wi-Fi connected"
#define CD_WIFI_SSID_NOT_FOUND		 51
#define RC_WIFI_SSID_NOT_FOUND		"51 Wi-Fi ssid not found"
#define CD_WIFI_AUTH_FAIL	    	 52
#define RC_WIFI_AUTH_FAIL		    "52 Wi-Fi authentication failed"
#define CD_WIFI_DISCONNECTED		 53
#define RC_WIFI_DISCONNECTED		"53 Wi-Fi disconnected"
#define CD_ERROR					 90
#define RC_ERROR					"90 Error"

#endif //SLAPPYBELL_FIRMWARE_STATUS_CODE_H
