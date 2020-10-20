#include "x86.h"
#include "device.h"

#define MAX_ROW 100

int displayRow = 0; //TODO futher extend, display position, in .bss section, not in .data section
int displayCol = 0;
int baseRow = 0;

uint16_t displayMem[80*MAX_ROW];

void initVga() {
	displayRow = 0;
	displayCol = 0;
	baseRow = 0;
	uint16_t data = 0 | (0x0f << 8);
	for (int i = 0; i < 80 * 25; i++) {
		int pos = i * 2;
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
	}
	for (int i = 0; i < 80 * MAX_ROW; i++) {
		displayMem[i] = data;
	}
	updateCursor(0, 0);
}

void clearScreen() {
	int i = 0;
	uint16_t data = 0 | (0x0f << 8);
	for (i = 0; i < 80 * MAX_ROW; i++) {
		displayMem[i] = data;
	}
	displayCol = 0;
	displayRow = 0;
	baseRow = 0;
	updateScreen();
	updateCursor(0,0);
}

void updateCursor(int row, int col){
	int cursorPos = row * 80 + col;
	outByte(0x3d4, 0x0f);
	outByte(0x3d5, (unsigned char)(cursorPos & 0xff));

	outByte(0x3d4, 0x0e);
	outByte(0x3d5, (unsigned char)((cursorPos>>8) & 0xff));
}

void updateScreen() {
	int i = 0;
	int pos = 0;
	uint16_t data = 0;
	int basePos = baseRow * 80;
	for (i = 0; i < 80 * 25; i++) {
		pos = i * 2;
		data = displayMem[i+basePos];
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
	}
}

void scrollScreen() {
	int i = 0;
	uint16_t data = 0;
	for (i = 80; i < 80 * MAX_ROW; i++) {
		displayMem[i-80] = displayMem[i];
	}
	data = 0 | (0x0f << 8);
	for (i = 80 * (MAX_ROW - 1); i < 80 * MAX_ROW; i++) {
		displayMem[i] = data;
	}
}

void writeVga(uint16_t data) {
	uint8_t character = data & 0xff;
	if (character == '\n') {
		displayRow++;
		displayCol = 0;
		if (displayRow == MAX_ROW){
			displayRow = MAX_ROW - 1;
			displayCol = 0;
			scrollScreen();
		}
	}
	else if (character == '\t') {
		do {
			displayCol++;
		} while (displayCol % 4 != 0);
		if (displayCol >= 80) {
			displayRow++;
			displayCol = 0;
			if (displayRow == MAX_ROW){
				displayRow = MAX_ROW - 1;
				displayCol = 0;
				scrollScreen();
			}
		}
	}
	else if (character == '\b') {
		if (displayCol > 0)
			displayCol--;
	}
	else {
		int pos = 80 * displayRow + displayCol;
		displayMem[pos] = data;
		displayCol++;
		if (displayCol == 80){
			displayRow++;
			displayCol = 0;
			if (displayRow == MAX_ROW){
				displayRow = MAX_ROW - 1;
				displayCol = 0;
				scrollScreen();
			}
		}
	}
	baseRow = displayRow - 24;
	if (baseRow < 0) baseRow = 0;
	updateScreen();
	int cursorRow = (displayRow > 24) ? 24: displayRow;
	updateCursor(cursorRow, displayCol);
}

void eraseVga() {
	displayCol--;
	if (displayCol < 0) {
		if (displayRow > 0) {
			displayRow--;
			displayCol = 79;
		}
		else
			displayCol = 0;
	}
	uint16_t data = 0 | (0x0f << 8);
	int pos = 80 * displayRow + displayCol;
	displayMem[pos] = data;
	updateScreen();
	//updateCursor(displayRow, displayCol);
	int cursorRow = (displayRow > 24) ? 24: displayRow;
	updateCursor(cursorRow, displayCol);
}