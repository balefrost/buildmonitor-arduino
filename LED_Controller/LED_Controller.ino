#include <SPI.h>
#include "RgbColor.h"

typedef RgbColor<unsigned byte> rgb8;

const rgb8 goodBuild(0x00, 0xff, 0x00);
const rgb8 badBuild(0xff, 0x00, 0x00);
const rgb8 uncertainBuild(0xff, 0xff, 0x00);

typedef struct {
	rgb8 color;
	bool building;
} BuildState;

BuildState buildStates[32];

void setup() {
	for (size_t i = 0; i < 32; ++i) {
		buildStates[i].color = rgb8(0x00, 0x00, 0x40);
		buildStates[i].building = true;
	}

	buildStates[0].color = goodBuild;
	buildStates[0].color = rgb8(0, 0, 0);
	buildStates[2].color = badBuild;
	buildStates[3].color = goodBuild;
	buildStates[4].color = uncertainBuild;
	buildStates[5].color = goodBuild;
	buildStates[6].color = goodBuild;
	buildStates[7].color = badBuild;

	buildStates[0].building = true;
	buildStates[1].building = false;
	buildStates[4].building = true;
	buildStates[5].building = true;
	buildStates[7].building = true;

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

void handleByte(byte b) {
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
				buildStates[commandAddress].building = commandByte == 'B' || commandByte == 'b';
				buildStates[commandAddress].color = commandColor;
			}

			commandAddress = 0;
			commandState = awaitingAddress;
			break;
		}
	}
}

const unsigned long cycleTime = 2000;

void loop() {
	int b;
	while((b = Serial.read()) >= 0) {
		handleByte(b);
	}

	byte buffer[32 * 3];
	for (size_t i = 0, idx = 0; i < 32; ++i, idx += 3) {
		BuildState buildState = buildStates[i];
		if (buildState.building) {
			float osc = (sin((millis() - cycleTime / 2.0 * i / 31) * 2 * M_PI / cycleTime) * 0.475) + 0.525;
			buffer[idx] = buildState.color.b * osc;
			buffer[idx + 1] = buildState.color.g * osc;
			buffer[idx + 2] = buildState.color.r * osc;
		} else {
			buffer[idx] = buildState.color.b;
			buffer[idx + 1] = buildState.color.g;
			buffer[idx + 2] = buildState.color.r;
		}
	}

	digitalWrite(12, 0);
	for (size_t i = 0; i < 32 * 3; ++i) {
		SPI.transfer(buffer[i]);
	}
	digitalWrite(12, 1);
	delay(33);
}