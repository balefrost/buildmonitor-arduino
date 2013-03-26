#include <SPI.h>

byte rule = 30;

uint32_t oldBits;
uint32_t newBits;

const unsigned long frameDuration = 1000;

unsigned long thisFlip;

inline bool setBit(uint32_t &var, size_t bit, bool state) {
	bool result = (var >> bit) & 0x01;
	if (state) {
		var |= (0x01 << bit);
	} else {
		var &= (0x01 << bit);
	}
	return result;
}

inline bool getBit(uint32_t &var, size_t bit) {
	return (var >> bit) & 0x01;
}

uint32_t calculateNewBits(uint32_t bits) {
	uint32_t result = 0;
	for (int i = 31; i >= 0; --i) {
		uint32_t neighborhood;
		if (i == 0) {
			neighborhood = ((bits << 1) & 0x06) | ((bits >> 31) & 0x01);
		} else if (i == 31) {
			neighborhood = ((bits << 2) & 0x04) | ((bits >> 30) & 0x03);
		} else {
			neighborhood = (bits >> (i - 1)) & 0x07;
		}

		bool newOn = (rule >> neighborhood) & 0x01;
		result = (result << 1) | (newOn ? 0x01 : 0x00);
	}

	return result;
}

void setup() {
	randomSeed(analogRead(0));
	oldBits = random(0, 0xffff) << 16 | random(0, 0xffff);
	newBits = calculateNewBits(oldBits);

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
		oldBits = newBits;
		newBits = calculateNewBits(newBits);
		thisFlip += frameDuration;
	}

	float progress = float(now - thisFlip) / frameDuration;
	float blend = 3 * progress * progress - 2 * progress * progress * progress;
	byte buffer[32 * 3];
	for (size_t i = 0, idx = 0; i < 32; ++i, idx += 3) {
		byte oldBit = getBit(oldBits, i) ? 0xff : 0x00;
		byte newBit = getBit(newBits, i) ? 0xff : 0x00;
		buffer[idx] = (byte)(blend * newBit + (1 - blend) * oldBit);
		buffer[idx + 1] = 0x00;
		buffer[idx + 2] = 0x00;
	}

	digitalWrite(12, 0);
	for (size_t i = 0; i < 32 * 3; ++i) {
		SPI.transfer(buffer[i]);
	}
	digitalWrite(12, 1);
	delay(1);
}