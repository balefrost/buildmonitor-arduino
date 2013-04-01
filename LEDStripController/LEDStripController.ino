#include <SPI.h>
#include "RgbColor.h"

typedef RgbColor<unsigned byte> rgb8;

const rgb8 rgbBlack(0x00, 0x00, 0x00);
const rgb8 goodBuild(0x00, 0xff, 0x00);
const rgb8 badBuild(0xff, 0x00, 0x00);
const rgb8 uncertainBuild(0xff, 0xff, 0x00);

const unsigned long cycleTime = 1000;
const unsigned long transitionDuration = 500;

class PixelState {
public:
	rgb8 getCurrentColor(unsigned long now) {
		float osc = 0;
		if (newPulsing || previousPulsing) {
			osc = (sin((now - cycleOffset) * 2 * M_PI / cycleTime) * 0.475) + 0.525;
		}

		rgb8 modulatedPreviousColor = previousPulsing ? osc * previousColor : previousColor;
		rgb8 modulatedNewColor = newPulsing ? osc * newColor : newColor;

		if (transitioning) {
			if (now - transitionStartTime < transitionDuration) {
				float brightness = 1 - float(now - transitionStartTime) / transitionDuration;
				return brightness * modulatedPreviousColor;
			} else if (now - transitionStartTime < 2 * transitionDuration) {
				return rgbBlack;
			} else if (now - transitionStartTime < 3 * transitionDuration) {
				float brightness = float(now - (transitionStartTime + 2 * transitionDuration)) / transitionDuration;
				return brightness * modulatedNewColor;
			} else {
				transitioning = false;
				return modulatedNewColor;
			}
		} else {
			return modulatedNewColor;
		}
	}

	void transition(unsigned long now, rgb8 newColor, bool newPulsing) {
		if (transitioning) {
			if (now - transitionStartTime >= 2 * transitionDuration && now - transitionStartTime < 3 * transitionDuration) {
				unsigned long milliProgress = now - (transitionStartTime + 2 * transitionDuration);
				this->transitionStartTime = now - (transitionDuration - milliProgress);
				this->previousColor = this->newColor;
				this->previousPulsing = this->newPulsing;
			}
		} else {
			this->previousColor = this->newColor;
			this->previousPulsing = this->newPulsing;
			this->transitionStartTime = now;
		}
		this->newColor = newColor;
		this->newPulsing = newPulsing;
		this->transitioning = true;
	}

	unsigned long cycleOffset;

	bool transitioning;
	unsigned long transitionStartTime;
	rgb8 previousColor;
	bool previousPulsing;

	rgb8 newColor;
	bool newPulsing;
};

PixelState pixelStates[32];

void setup() {
	for (size_t i = 0; i < 32; ++i) {
		pixelStates[i].newColor = rgb8(0x00, 0x00, 0x40);
		pixelStates[i].newPulsing = true;
		pixelStates[i].cycleOffset = i * 20;
	}

	pixelStates[0].newColor = goodBuild;
	pixelStates[1].newColor = rgb8(0, 0, 0);
	pixelStates[2].newColor = badBuild;
	pixelStates[3].newColor = goodBuild;
	pixelStates[4].newColor = uncertainBuild;
	pixelStates[5].newColor = goodBuild;
	pixelStates[6].newColor = goodBuild;
	pixelStates[7].newColor = badBuild;

	pixelStates[0].newPulsing = true;
	pixelStates[1].newPulsing = false;
	pixelStates[4].newPulsing = false;
	pixelStates[5].newPulsing = true;
	pixelStates[7].newPulsing = false;

	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV8);
	SPI.begin();

	pinMode(SS, OUTPUT);
	pinMode(12, OUTPUT);

	Serial.begin(9600);
}

enum SerialState {
	awaitingAddress,
	awaitingColor0,
	awaitingColor1,
	awaitingColor2,
	awaitingColor3,
	awaitingColor4,
	awaitingColor5,
	awaitingColor6
};

SerialState commandState;
byte commandAddress;
byte commandByte;
rgb8 commandColor;

int8_t parseHex(byte b) {
	if (b >= '0' && b <= '9') {
		return b - '0';
	}

	if (b >= 'a' && b <= 'f') {
		return b - 'a' + 10;
	}

	if (b >= 'A' && b <= 'F') {
		return b - 'A' + 10;
	}

	return -1;
}

void handleByte(unsigned long now, byte b) {
	switch(commandState) {
		case awaitingAddress: {
			if (b >= '0' && b <= '9') {
				commandAddress = commandAddress * 10 + (b - '0');
			} else {
				commandByte = b;
				commandState = awaitingColor0;
				commandColor.r = 0;
				commandColor.g = 0;
				commandColor.b = 0;
			}
			break;
		}
		case awaitingColor0: {
			commandColor.r |= (0x0f & parseHex(b)) << 4;
			commandState = awaitingColor1;
			break;
		}
		case awaitingColor1: {
			commandColor.r |= (0x0f & parseHex(b));
			commandState = awaitingColor2;
			break;
		}
		case awaitingColor2: {
			commandColor.g |= (0x0f & parseHex(b)) << 4;
			commandState = awaitingColor3;
			break;
		}
		case awaitingColor3: {
			commandColor.g |= (0x0f & parseHex(b));
			commandState = awaitingColor4;
			break;
		}
		case awaitingColor4: {
			commandColor.b |= (0x0f & parseHex(b)) << 4;
			commandState = awaitingColor5;
			break;
		}
		case awaitingColor5: {
			commandColor.b |= (0x0f & parseHex(b));
			
			if (commandAddress >= 0 && commandAddress <= 31) {
				pixelStates[commandAddress].transition(now, commandColor, commandByte == 'B' || commandByte == 'b');
			}

			commandAddress = 0;
			commandState = awaitingAddress;
			break;
		}
	}
}

void loop() {
	unsigned long now = millis();

	int b;
	while((b = Serial.read()) >= 0) {
		handleByte(now, b);
	}

	byte buffer[32 * 3];
	for (size_t i = 0, idx = 0; i < 32; ++i, idx += 3) {
		PixelState &pixelState = pixelStates[i];
		rgb8 currentColor = pixelState.getCurrentColor(now);
		buffer[idx] = currentColor.b;
		buffer[idx + 1] = currentColor.g;
		buffer[idx + 2] = currentColor.r;
	}

	digitalWrite(12, 0);
	for (size_t i = 0; i < 32 * 3; ++i) {
		SPI.transfer(buffer[i]);
	}
	digitalWrite(12, 1);
	delay(33);
}