#include <types.h>
#include <device/vga.h>

#include "terminal.h"

uint8_t terminal_startCol;
uint8_t terminal_row;
uint8_t terminal_column;
uint8_t terminal_colour;
volatile uint16_t* terminal_buffer;
 
void terminal_initialize(bool clearMem) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_colour = vga_make_colour(VGA_COLOUR_LIGHT_GREY, VGA_COLOUR_BLACK);

	terminal_buffer = (uint16_t *) VGA_TEXTMEM;

	if(clearMem) {
		for (int y = 0; y < VGA_TEXT_ROWS; y++) {
			for (int x = 0; x < VGA_TEXT_COLS; x++) {
				uint16_t index = (y * VGA_TEXT_COLS) + x;
				terminal_buffer[index] = vga_txt_makentry(' ', terminal_colour);
			}
		}
	}

	terminal_colour = vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLACK);
}
 
void terminal_setColour(uint8_t colour) {
	terminal_colour = colour;
}
 
void terminal_putentryat(char c, uint8_t color, uint8_t x, uint8_t y) {
	const uint16_t index = (y * VGA_TEXT_COLS) + x;
	terminal_buffer[index] = vga_txt_makentry(c, color);
}
 
void terminal_putchar(char c) {
	if(c == '\n') {
		terminal_row++;
		terminal_column = terminal_startCol;
	} else {
		terminal_putentryat(c, terminal_colour, terminal_column, terminal_row);

		if (++terminal_column == VGA_TEXT_COLS) {
			terminal_column = 0;
			if (++terminal_row == VGA_TEXT_ROWS) {
				terminal_row = 0;
			}
		}
	}
}

void terminal_write_string(char *string) {
	size_t datalen = strlen(string);
	
	for(int i = 0; i < datalen; i++) {
		terminal_putchar(string[i]);
	}

	terminal_startCol = terminal_column;
}

void terminal_write_byte(uint8_t byte) {
	uint8_t nybble = (byte & 0xF0) >> 0x04;
	if(nybble > 0x09) { // ASCII characters - A
		nybble += 0x37;
	} else { // ASCII numbers
		nybble += 0x30;		
	}

	terminal_putchar((unsigned char) nybble);

	nybble = (byte & 0x0F);
	if(nybble > 0x09) { // ASCII characters - A
		nybble += 0x37;
	} else { // ASCII numbers
		nybble += 0x30;		
	}

	terminal_putchar((unsigned char) nybble);
}

void terminal_write_word(uint16_t word) {
	terminal_write_byte((word & 0xFF00) >> 0x8);
	terminal_write_byte(word & 0x00FF);
}

void terminal_write_dword(uint32_t dword) {
	terminal_write_word((dword & 0xFFFF0000) >> 0x10);
	terminal_write_word(dword & 0x0000FFFF);
}

void terminal_write_int(int value) {
   terminal_write_dword(value);
}

void terminal_clear() {
	for (int y = 0; y < VGA_TEXT_ROWS; y++) {
		for (int x = 0; x < VGA_TEXT_COLS; x++) {
			uint16_t index = (y * VGA_TEXT_COLS) + x;
			terminal_buffer[index] = vga_txt_makentry(' ', terminal_colour);
		}
	}	

	terminal_row = terminal_column = 0;
}

void terminal_setPos(uint8_t col, uint8_t row) {
	terminal_column = col;
	terminal_row = row;	
}
