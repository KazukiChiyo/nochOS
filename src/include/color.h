/* author: Kexuan Zou
external source: http://wiki.osdev.org/Text_UI */
#ifndef _COLOR_H
#define _COLOR_H

#define FORE_COLOR(c) ((c))
#define BACK_COLOR(c) ((c) << 4)

#define SET_COLOR(f, b) \
    (FORE_COLOR((f)) | BACK_COLOR((b)))

#define BLACK       0x0
#define BLUE        0x1
#define GREEN       0x2
#define CYAN        0x3
#define RED         0x4
#define MAGEN       0x5
#define BROWN       0x6
#define LIGHTGRAY   0x7
#define DARKGRAY    0x8
#define LIGHTBLUE   0x9
#define LIGHTGREEN  0xA
#define LIGHTCYAN   0xB
#define LIGHTRED    0xC
#define PINK        0xD
#define YELLOW      0xE
#define WHITE       0xF

extern uint8_t ATTRIB;

#endif
