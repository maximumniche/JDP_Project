#pragma once
#include <Arduino.h>

// D8 = Serial1 TX → LCD UART
void lcd_setup();
void lcd_update(float pitchVal, float srVal, float modVal);
void setBaud(int baudRate);

void goToPos(int x, int y);
void writeAtPos(int x, int y, int symbol);
void writeStrAtPos(int x, int y, String data);
void drawColLine(int column, int symbol);
void drawRowLine(int row, int symbol);
int  coordsToPos(int x, int y);
