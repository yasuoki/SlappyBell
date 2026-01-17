//
// Created by Yasuoki on 2025/07/25.
//

#ifndef SLAPPYBELL_FIRMWARE_LED_SEQUENCER_H
#define SLAPPYBELL_FIRMWARE_LED_SEQUENCER_H

#include <Arduino.h>

typedef struct _LED_SEQUENCE {
	byte R;
	byte G;
	byte B;
	float deltaR;
	float deltaG;
	float deltaB;
	uint32_t	time;
} LED_SEQUENCE;

class LedSequencer {
private:
	int 			_ledIndex;
	LED_SEQUENCE 	_sequence[MAX_LED_SEQUENCE_LENGTH];
	size_t			_sequenceCount;
	size_t			_sequencePtr;
	uint32_t 		_lastTime;
	byte _r;
	byte _g;
	byte _b;

	static LedSequencer _ledSequencer[SLOT_COUNT];
	const char *_parseColorSegment(const char *ptr, LED_SEQUENCE *seg);
	bool _parse(const char *pattern);
	bool _reset();
	bool _update(uint32_t now);
public:
	LedSequencer();
	static void init();
	static void clear(int index = -1);
	static void update(uint32_t now);
	static bool parse(int index, const char *pattern);
};


#endif // SLAPPYBELL_FIRMWARE_LED_SEQUENCER_H
