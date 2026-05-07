#include "lcd.h"

int coordsToPos(int x, int y) {
    int rowOffsets[] = {0x00, 0x40, 0x14, 0x54};
    return rowOffsets[y] + x;
}

void goToPos(int x, int y) {
    Serial1.write(0xFE);
    Serial1.write(0x45);
    Serial1.write(coordsToPos(x, y));
}

void writeStrAtPos(int x, int y, String data) {
    goToPos(x, y);
    Serial1.print(data);
}

void writeAtPos(int x, int y, int symbol) {
    goToPos(x, y);
    Serial1.write(symbol);
}

void drawColLine(int column, int symbol) {
    for (int i = 0; i < 4; i++) writeAtPos(column, i, symbol);
}

void drawRowLine(int row, int symbol) {
    for (int i = 0; i < 20; i++) writeAtPos(i, row, symbol);
}

void lcd_setup() {
    // clear screen
    Serial1.write(0xFE);
    Serial1.write(0x51);

    // header row: Pitch / SR / Mod labels
    Serial1.write(0xFE);
    Serial1.write(0x45);
    Serial1.write(0x41);
    Serial1.write("Pitch ");
    Serial1.write("SR    ");
    Serial1.write("Mod  ");

    drawRowLine(0, '=');
    drawRowLine(3, '=');
    drawColLine(0,  '|');
    drawColLine(6,  '|');
    drawColLine(12, '|');
    drawColLine(19, '|');

    writeAtPos(0,  0, 0xAF);
    writeAtPos(19, 3, 0xAF);
}

void lcd_update(float pitchVal, float srVal, float modVal) {
    String p = String(pitchVal, 2).substring(0, 5);
    String s = String(srVal,    2).substring(0, 5);
    String m = String(modVal,   2).substring(0, 5);

    writeStrAtPos(1,  2, p);
    writeStrAtPos(7,  2, s);
    writeStrAtPos(13, 2, m);
}
