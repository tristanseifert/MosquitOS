#include <types.h>
#include "fb_console.h"

#include "svga.h"

#include "io/io.h"
#include "rsrc/ter-i16b.h"
#include "rsrc/ter-i16n.h"

// Pointers to fonts
static uint8_t *font_reg, *font_bold;
// Video mode info
static uint16_t width, height, depth, bytesPerLine;
static uint32_t video_base, video_size;
// Cursor location (in text cells)
static uint16_t col, row;
// Set by escape sequences
static uint8_t fg_colour, bg_colour;
// Used to parse escape codes
static bool next_char_is_escape_seq, is_bold;

// Colour code -> 16bpp
static uint16_t fb_console_col_map[16] = {
	0x0000, 0xFFFF, 0xF800, 0x07E0, 
	0x001F, 0x0000, 0x0000, 0x0000,

	// 0x08-0x0F are black (unused)
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};

/*
 * Initialises the framebuffer console
 */
void fb_console_init() {
	svga_mode_info_t *svga_mode_info = svga_mode_get_info(SVGA_DEFAULT_MODE);
	video_base = svga_map_fb(svga_mode_info->physbase);

	bytesPerLine = svga_mode_info->pitch;
	width = svga_mode_info->screen_width;
	height = svga_mode_info->screen_height;
	depth = svga_mode_info->bpp / 8;

	fb_console_set_font(&ter_i16n_raw, &ter_i16b_raw);

	uint16_t *videoMem = (uint16_t *) video_base;

	for(int i = 0; i < ((bytesPerLine / depth) * height); i++) {
		videoMem[i] = fb_console_col_map[0x00];
	}

	is_bold = false;
	next_char_is_escape_seq = false;
	fg_colour = 0x01;
	bg_colour = 0x00;

	//kprintf("Screen mode %ix%i, %i bpp\n", width, height, svga_mode_info->bpp);
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
			fg_colour = c;
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
		uint16_t *write_ptr = (uint16_t *) video_base + ((row * CHAR_HEIGHT) * (bytesPerLine / depth)) + (col * CHAR_WIDTH);

		static const uint8_t x_to_bitmap[CHAR_WIDTH] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

		for(uint8_t y = 0; y < CHAR_HEIGHT; y++) {
			uint8_t fontChar = read_ptr[y];

			for(uint8_t x = 0; x < CHAR_WIDTH; x++) {
				if(x_to_bitmap[x] & fontChar) {
					write_ptr[x] = fb_console_col_map[fg_colour];
				} else {
					write_ptr[x] = fb_console_col_map[bg_colour];
				}
			}
		
			// go to next line
			write_ptr += (bytesPerLine / depth);
		}

		col++;

		if(col > (width / CHAR_WIDTH)) {
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
void fb_console_set_colour(uint8_t index, uint32_t rgb) {
	io_outb(0x3C8, index);
	io_outb(0x3C9, (rgb >> 0x10) & 0xFF);
	io_outb(0x3C9, (rgb >> 0x08) & 0xFF);
	io_outb(0x3C9, rgb & 0xFF);
}