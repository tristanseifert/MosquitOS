#include <types.h>

#include "vga_fonts.h"
#include "vga_modes.h"
#include "vga.h"

uint16_t vga_txt_makentry(unsigned char c, uint8_t color) {
	return (c | color << 8);
}
