//
// Created by Yasuoki on 2025/07/25.
//

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "led_sequencer.h"

#include "processor.h"
#include "utils.h"

Adafruit_NeoPixel pixels(SLOT_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
LedSequencer LedSequencer::_ledSequencer[SLOT_COUNT];

LedSequencer::LedSequencer() {
	_ledIndex = -1;
	_sequenceCount = 0;
	_sequencePtr = 0;
	_lastTime = 0;
	_r = 0;
	_g = 0;
	_b = 0;
}

const char *LedSequencer::_parseColorSegment(const char *ptr, LED_SEQUENCE *seg) {
	uint32_t rgb;
	ptr = Utils::parseHex(ptr, &rgb);
	if (!ptr)
		return nullptr;
	int time = 1000;
	if (*ptr == ':') {
		ptr = Utils::parseInt(++ptr, &time);
		if (!ptr)
			return nullptr;
	}
	seg->R = (rgb >> 16) & 0xFF;
	seg->G = (rgb >> 8) & 0xFF;
	seg->B = rgb & 0xFF;
	seg->time = time;
	return ptr;
}

// RGB,RGB		1sec color change
// RGB:50,RGB	50msec color change
// RGB>RGB		1sec gradient
// RGB:50>RGB	50msec gradient
bool LedSequencer::_parse(const char *ptr) {
	_reset();
	ptr = Utils::skipWs(ptr);
	if (!ptr)
		return false;
	int segs = 0;
	while (*ptr && segs < MAX_LED_SEQUENCE_LENGTH) {
		if (!Utils::isHex(*ptr))
			break;
		LED_SEQUENCE *seg = &_sequence[segs++];
		ptr = _parseColorSegment(ptr, seg);
		if (!ptr)
			return false;
		if (*ptr == '>') {
			seg->deltaR = 1;
			ptr++;
		} else {
			seg->deltaR = 0;
			seg->deltaG = 0;
			seg->deltaB = 0;
			if (*ptr == ',')
				ptr++;
		}
	}
	if (segs == 0)
		return false;
	for (int i = 0; i < segs-1; i++) {
		LED_SEQUENCE *seg = &_sequence[i];
		if (seg->deltaR == 1) {
			int diffR = (seg+1)->R - seg->R;
			int diffG = (seg+1)->G - seg->G;
			int diffB = (seg+1)->B - seg->B;
			seg->deltaR = (float)diffR / seg->time;
			seg->deltaG = (float)diffG / seg->time;
			seg->deltaB = (float)diffB / seg->time;
		}
	}
	if (segs > 1) {
		LED_SEQUENCE *seg = &_sequence[segs-1];
		if (seg->deltaR == 1) {
			int diffR = _sequence[0].R - seg->R;
			int diffG = _sequence[0].G - seg->G;
			int diffB = _sequence[0].B - seg->B;
			seg->deltaR = (float)diffR / seg->time;
			seg->deltaG = (float)diffG / seg->time;
			seg->deltaB = (float)diffB / seg->time;
		}
	} else {
		_sequence[0].time = 0;
		_sequence[0].deltaR = 0;
		_sequence[0].deltaG = 0;
		_sequence[0].deltaB = 0;
	}
	_sequenceCount = segs;

//	for (int i = 0; i < segs; i++) {
//		Processor::sendLog("sequence[%d]=%6.6lx:%6.6lx:%6.6lx time=%lu", i, _sequence[i].R, _sequence[i].G, _sequence[i].B, _sequence[i].time);
//	}

	return true;
}

bool LedSequencer::_reset() {
	_sequencePtr = 0;
	_sequenceCount = 0;
	_lastTime = 0;
	if (_r != 0 || _g != 0 || _b != 0) {
		_r = 0;
		_g = 0;
		_b = 0;
		pixels.setPixelColor(_ledIndex, 0);
		return true;
	}
	return false;
}

bool LedSequencer::_update(uint32_t now) {
	if(_sequenceCount > 0) {
		byte r = _sequence[_sequencePtr].R;
		byte g = _sequence[_sequencePtr].G;
		byte b = _sequence[_sequencePtr].B;
		if (_lastTime != 0) {
			uint32_t dt = now -_lastTime;
			if ( _sequence[_sequencePtr].time <= dt) {
				_sequencePtr++;
				if(_sequencePtr >= _sequenceCount) {
					_sequencePtr = 0;
				}
				r = _sequence[_sequencePtr].R;
				g = _sequence[_sequencePtr].G;
				b = _sequence[_sequencePtr].B;
				_lastTime = now;
			} else {
				if (_sequence[_sequencePtr].deltaR != 0)
					r += (byte)((double)_sequence[_sequencePtr].deltaR * dt);
				if (_sequence[_sequencePtr].deltaG != 0)
					g += (byte)((double)_sequence[_sequencePtr].deltaG * dt);
				if (_sequence[_sequencePtr].deltaB != 0)
					b += (byte)((double)_sequence[_sequencePtr].deltaB * dt);
			}
		}
		else {
			_lastTime = now;
		}
		if (_r != r || _g != g || _b != b) {
			_r = r;
			_g = g;
			_b = b;
			pixels.setPixelColor(_ledIndex, Adafruit_NeoPixel::Color(r,g,b));
			return true;
		}
	}
	return false;
}

void LedSequencer::init() {
	pixels.begin();
	pixels.clear();
	pixels.show();
	for (int i = 0; i < SLOT_COUNT; i++) {
		_ledSequencer[i]._ledIndex = i;
	}

	_ledSequencer[5]._parse("3333CC:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>");
	_ledSequencer[4]._parse("000000:2000>3333CC:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>3333CC:2000>");
	_ledSequencer[3]._parse("000000:2000>000000:2000>3333CC:2000>000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>3333CC:2000>000000:2000>");
	_ledSequencer[2]._parse("000000:2000>000000:2000>000000:2000>3333CC:2000>000000:2000>000000:2000>000000:2000>3333CC:2000>000000:2000>000000:2000>");
	_ledSequencer[1]._parse("000000:2000>000000:2000>000000:2000>000000:2000>3333CC:2000>000000:2000>3333CC:2000>000000:2000>000000:2000>000000:2000>");
	_ledSequencer[0]._parse("000000:2000>000000:2000>000000:2000>000000:2000>000000:2000>3333CC:2000>000000:2000>000000:2000>000000:2000>000000:2000>");

}

void LedSequencer::clear(int index) {
	if (index != -1) {
		if (_ledSequencer[index]._reset()) {
			pixels.show();
		}
	} else {
		bool update = false;
		for (int i = 0; i < SLOT_COUNT; i++) {
			if(_ledSequencer[i]._reset())
				update = true;
		}
		if (update)
			pixels.show();
	}
}

void LedSequencer::update(uint32_t now) {
	bool updated = false;
	for (int i = 0; i < SLOT_COUNT; i++) {
		if (_ledSequencer[i]._update(now))
			updated = true;
	}
	if (updated) {
		pixels.show();
	}
}

bool LedSequencer::parse(int index, const char *pattern) {
	return _ledSequencer[index]._parse(pattern);
}
