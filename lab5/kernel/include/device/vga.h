#ifndef __VGA_H__
#define __VGA_H__

void initVga();
void clearScreen();
void updateCursor(int row, int col);
void scrollScreen();
void updateScreen();
void writeVga(uint16_t data);
void eraseVga();

#endif
