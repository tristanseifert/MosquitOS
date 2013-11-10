#include <types.h>
#include "fb_console.h"

#include "io/io.h"
#include "rsrc/ter-i16b.h"
#include "rsrc/ter-i16n.h"

// Pointers to fonts
uint8_t *font_reg, *font_bold;
// Video mode info
uint16_t width, height, depth;
uint32_t video_base, video_size;
// Cursor location (in text cells)
uint16_t col, row;
// Set by escape sequences
uint8_t colour;
// Used to parse escape codes
bool next_char_is_escape_seq, is_bold;

static uint32_t fb_console_col_map[16] = {
	0x00000000, 0x00FFFFFF, 0x00FF0000, 0x0000FF00, 
	0x000000FF, 0x00FFFF00, 0x0000FFFF, 0x00FF00FF,

	// 0x08-0x0F are black (unused)
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};

/*
 * Initialises the framebuffer console
 */
void fb_console_init() {
	width = 640;
	height = 480;
	depth = 1;
	video_base = 0x0A0000;
	video_size = 0x10000;

	fb_console_set_font(&ter_i16n_raw, &ter_i16b_raw);

	uint8_t *vgaMem = (uint8_t *) video_base;

	for(int i = 0; i < (width*height*depth); i++) {
		vgaMem[i] = 0x00;
	}

	is_bold = 0;
	next_char_is_escape_seq = 0;
	colour = 0x01;

	for(uint8_t i = 0; i < 16; i++) {
		fb_console_set_col(i, fb_console_col_map[i]);
	}
}

/*
 * Prints a character to the framebuffer console.
 */
void fb_console_putchar(unsigned char c) {
	if(c == 0x01) {
		next_char_is_escape_seq = true;
		return;
	}

	// Check if the following character is part of an escape sequence
	if(next_char_is_escape_seq) {
		// Codes 0x00 to 0x0F are colours
		if(c >= 0x00 && c <= 0x0F) {
			colour = c;
			next_char_is_escape_seq = false;
		} else if(c == 0x10 || c == 0x11) {
			is_bold = (c == 0x11) ? true : false;

			next_char_is_escape_seq = false;
		} else {
			next_char_is_escape_seq = false;
		}
	} else { // Handle printing of a regular character
		// Characters are 16 px tall, i.e. 0x10 bytes in stored rep
		uint8_t *read_ptr = ((is_bold) ? font_bold : font_reg) + (c * CHAR_HEIGHT);
		uint8_t *write_ptr = (uint8_t *) video_base + ((row * CHAR_HEIGHT) * width * depth) + ((col * CHAR_WIDTH) * depth);

		for(int y = 0; y < CHAR_HEIGHT; y++) {
			for(int x = 0; x < CHAR_WIDTH; x++) {
				uint8_t bit = (1 << (CHAR_WIDTH - x)) & read_ptr[y];
				
				if(bit) {
					write_ptr[x] = colour;
				} else {
					write_ptr[x] = 0x00;
				}
			}
		
			// go to next line
			write_ptr += (width * depth);

			// switch banks
			if(((uint32_t) write_ptr) > (video_base + video_size)) {
				write_ptr = (uint8_t *) video_base;
			}
		}

		col++;

		if(col >= (width / CHAR_WIDTH)) {
			row++;
			col = 0;
		}
	}
}

/*
 * Handles the character as a control character.
 */
void fb_console_control(unsigned char c) {
	switch(c) {
		case '\n':
			row++;
			col = 0;
			break;

		default:
			break;
	}
}

/*
 * Sets the regular and bold fonts. If a font pointer is NULL, it is ignored.
 */
void fb_console_set_font(void* reg, void* bold) {
	if(reg) font_reg = reg;
	if(bold) font_bold = bold;
}

/*
 * Maps a console colour to an RGB colour by changing VGA DAC.
 */
void fb_console_set_col(uint8_t index, uint32_t rgb) {
	io_outb(0x3C8, index);
	io_outb(0x3C9, (rgb >> 0x10) & 0xFF);
	io_outb(0x3C9, (rgb >> 0x08) & 0xFF);
	io_outb(0x3C9, rgb & 0xFF);
}