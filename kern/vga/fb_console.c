#include <types.h>
#include "fb_console.h"

#include "svga.h"

#include "io/io.h"
#include "rsrc/ter-i16b.h"
#include "rsrc/ter-i16n.h"

static void fb_console_putpixel(uint8_t* screen, int x, int y, uint32_t color);
static void fb_console_scroll_up(unsigned int num_rows);

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
static uint32_t fb_console_col_map[16] = {
	0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 
	0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,

	0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
	0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
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

	// Clear screen
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			fb_console_putpixel((uint8_t *) video_base, x, y, fb_console_col_map[0x00]);
		}
	}

	is_bold = false;
	next_char_is_escape_seq = false;
	fg_colour = 0x0F;
	bg_colour = 0x00;

	col = row = 0;
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

		static const uint8_t x_to_bitmap[CHAR_WIDTH] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
		static uint8_t fontChar;
		static uint32_t out;

		for(uint8_t y = 0; y < CHAR_HEIGHT; y++) {
			fontChar = read_ptr[y];

			// Process one column at a time
			for(uint8_t x = 0; x < CHAR_WIDTH; x++) {
				if(x_to_bitmap[x] & fontChar) {
					fb_console_putpixel((uint8_t *) video_base, x+(col*CHAR_WIDTH), y+(row*CHAR_HEIGHT), fb_console_col_map[fg_colour]);
				}/* else {
					fb_console_putpixel((uint8_t *) video_base, x+(col*CHAR_WIDTH), y+(row*CHAR_HEIGHT), fb_console_col_map[bg_colour]);
				}*/
			}
		}

		// Increment column and check row
		col++;

		if(col > (width / CHAR_WIDTH)) {
			fb_console_control('\n');
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

			if(row > (height / CHAR_HEIGHT)) {
				uint32_t *videoMem = (uint32_t *) video_base;
				uint32_t pixel = (fb_console_col_map[0x00] << 0x10) | fb_console_col_map[0x00];
				for(int i = 0; i < ((bytesPerLine / depth) * height)/8; i++) {
					videoMem[0] = pixel;
					videoMem[1] = pixel;
					videoMem[2] = pixel;
					videoMem[3] = pixel;

					videoMem += 4;
				}

				row = 0;
			}

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
 * Scrolls the display up number of rows
 */
static void fb_console_scroll_up(unsigned int num_rows) {
	uint32_t *read_ptr = (uint32_t *) video_base + ((num_rows * CHAR_HEIGHT) * (bytesPerLine / depth) / 2);
	uint32_t *write_ptr = (uint32_t *) video_base;

	int num_pixels = ((height - (num_rows * CHAR_HEIGHT)) * (bytesPerLine / depth)) / 2;

	for(int i = 0; i < num_pixels/4; i++) {
		*write_ptr++ = *read_ptr++;
		*write_ptr++ = *read_ptr++;
		*write_ptr++ = *read_ptr++;
		*write_ptr++ = *read_ptr++;
	}

	row -= num_rows;
}

/*
 * Plots a pixel in 24bpp mode.
 */
static void fb_console_putpixel(uint8_t* screen, int x, int y, uint32_t color) {
    int where = (x * depth) + (y * bytesPerLine);
    screen[where] = color & 255;              // BLUE
    screen[where + 1] = (color >> 8) & 255;   // GREEN
    screen[where + 2] = (color >> 16) & 255;  // RED
}