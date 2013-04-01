#include <SPI.h>
#include "RgbColor.h"

byte rule = 30;

struct State {
	uint32_t bits;
	unsigned long birthDates[32];
};

struct State oldState;
struct State newState;

const unsigned long frameDuration = 1000;

unsigned long thisFlip;

inline bool getBit(uint32_t var, size_t bit) {
	return (var >> bit) & 0x01;
}

rgb8 newColor(0xff, 0xff, 0x00);
rgb8 oldColor(0x80, 0x00, 0x00);

struct State calculateNewState(struct State oldState, unsigned long now) {
	struct State result;
	result.bits = 0;
	uint32_t oldBits = oldState.bits;
	for (int i = 31; i >= 0; --i) {
		uint32_t neighborhood;
		if (i == 0) {
			neighborhood = ((oldBits << 1) & 0x06) | ((oldBits >> 31) & 0x01);
		} else if (i == 31) {
			neighborhood = ((oldBits << 2) & 0x04) | ((oldBits >> 30) & 0x03);
		} else {
			neighborhood = (oldBits >> (i - 1)) & 0x07;
		}

		bool oldOn = getBit(oldBits, i);
		bool newOn = (rule >> neighborhood) & 0x01;
		result.bits = (result.bits << 1) | (newOn ? 0x01 : 0x00);
		result.birthDates[i] = !oldOn && newOn ? now : oldState.birthDates[i];
	}

	return result;
}

void setup() {
	randomSeed(analogRead(0));
	
	unsigned long now = millis();
	
	oldState.bits = random(0, 0xffff) << 16 | random(0, 0xffff);
	for (int i = 0; i < 32; ++i) {
		oldState.birthDates[i] = now;
	}
	newState = calculateNewState(oldState, now);

	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
	SPI.begin();

	pinMode(SS, OUTPUT);
	pinMode(12, OUTPUT);

	thisFlip = millis();
}

void loop() {
	unsigned long now = millis();

	while (now > (thisFlip + frameDuration)) {
		oldState = newState;
		newState = calculateNewState(oldState, now);
		thisFlip += frameDuration;
	}

	float progress = float(now - thisFlip) / frameDuration;
	float brightnessBlend = 3 * progress * progress - 2 * progress * progress * progress;
	byte buffer[32 * 3];
	for (size_t i = 0, idx = 0; i < 32; ++i, idx += 3) {
		float colorBlend = float(now - newState.birthDates[i]) / (3 * frameDuration);
		colorBlend = colorBlend > 1 ? 1 : colorBlend;
		rgb8 currentColor = newColor.interpolateTo(oldColor, colorBlend);
		
		byte oldBit = getBit(oldState.bits, i) ? 0xff : 0x00;
		byte newBit = getBit(newState.bits, i) ? 0xff : 0x00;

		rgb8 blendedColor;
		if (!oldBit & !newBit) {
			blendedColor = rgb8(0x00, 0x00, 0x00);
		} else if (oldBit && newBit) {
			blendedColor = currentColor;
		} else if (!oldBit && newBit) {
			blendedColor = rgb8(0, 0, 0).interpolateTo(currentColor, brightnessBlend);
		} else if (oldBit && !newBit) {
			blendedColor = currentColor.interpolateTo(rgb8(0, 0, 0), brightnessBlend);
		}
		
		buffer[idx] = blendedColor.b;
		buffer[idx + 1] = blendedColor.g;
		buffer[idx + 2] = blendedColor.r;
	}

	digitalWrite(12, 0);
	for (size_t i = 0; i < 32 * 3; ++i) {
		SPI.transfer(buffer[i]);
	}
	digitalWrite(12, 1);
	delay(1);
}