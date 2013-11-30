#include "vga_console.h"
#include "io/io.h"
#include <types.h>

#define VGA_TEXT_MEMORY 0xB8000

#define VGA_WIDTH 80
#define VGA_HEIGHT 24

static uint16_t terminal_row;
static uint16_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

static bool next_char_is_escape_seq, is_bold;

/* Hardware text mode color constants. */
enum vga_color {
	COLOR_BLACK = 0,
	COLOR_BLUE = 1,
	COLOR_GREEN = 2,
	COLOR_CYAN = 3,
	COLOR_RED = 4,
	COLOR_MAGENTA = 5,
	COLOR_BROWN = 6,
	COLOR_LIGHT_GREY = 7,
	COLOR_DARK_GREY = 8,
	COLOR_LIGHT_BLUE = 9,
	COLOR_LIGHT_GREEN = 10,
	COLOR_LIGHT_CYAN = 11,
	COLOR_LIGHT_RED = 12,
	COLOR_LIGHT_MAGENTA = 13,
	COLOR_LIGHT_BROWN = 14,
	COLOR_WHITE = 15,
};
 
static uint8_t make_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}
 
static uint16_t make_vgaentry(char c, uint8_t color) {
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

static void terminal_putentryat(char c, uint8_t color, uint16_t x, uint16_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = make_vgaentry(c, color);
}

/*
 * Updates the position of the VGA cursor.
 */
static void vga_update_cursor() {
	unsigned short position = (terminal_row * 80) + terminal_column;

	// cursor LOW port to vga INDEX register
	io_outb(0x3D4, 0x0F);
	io_outb(0x3D5, (unsigned char)(position & 0xFF));

	// cursor HIGH port to vga INDEX register
	io_outb(0x3D4, 0x0E);
	io_outb(0x3D5, (unsigned char )((position >> 8) & 0xFF));
}

/*
 * Initializes VGA console. (Clear text mode memory)
 */
void vga_console_init() {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
	terminal_buffer = (uint16_t*) VGA_TEXT_MEMORY;
	for (int y = 0; y < VGA_HEIGHT; y++) {
		for (int x = 0; x < VGA_WIDTH; x++) {
			int index = y * VGA_WIDTH + x;
			terminal_buffer[index] = make_vgaentry(' ', terminal_color);
		}
	}
}

/*
 * Puts a character on the screen.
 */
void vga_console_putchar(unsigned char c) {
	if(c == 0x01) {
		next_char_is_escape_seq = true;
		return;
	}

	// Check if the following character is part of an escape sequence
	if(next_char_is_escape_seq) {
		// Codes 0x00 to 0x0F are colours
		if(c >= 0x00 && c <= 0x0F) {
			terminal_color |= c & 0x0F;
			next_char_is_escape_seq = false;
		} else if(c == 0x10 || c == 0x11) {
			is_bold = (c == 0x11) ? true : false;

			next_char_is_escape_seq = false;
		} else {
			next_char_is_escape_seq = false;
		}
	} else {
		terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
		if (++terminal_column == VGA_WIDTH) {
			terminal_column = 0;
			if (++terminal_row == VGA_HEIGHT) {
				terminal_row = 0;
			}
		}

		vga_update_cursor();
	}
}

/*
 * Processes (or ignores) control sequences.
 */
void vga_console_control(unsigned char c) {
	if(c == '\n') {
		terminal_row++;
		terminal_column = 0;

		vga_update_cursor();
	}
}