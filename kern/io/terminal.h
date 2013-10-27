#ifndef TERMINAL_H
#define TERMINAL_H

#include <types.h>
#include "device/vga.h"

void terminal_initialize(bool clearMem);

void terminal_setColour(uint8_t colour);
void terminal_write_string(char *string);
void terminal_putchar(char c);
void terminal_setPos(uint8_t col, uint8_t row);
void terminal_clear();

void terminal_write_byte(uint8_t byte);
void terminal_write_word(uint16_t word);
void terminal_write_dword(uint32_t dword);
void terminal_write_int(int value);

#endif