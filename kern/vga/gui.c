#include <types.h>
#include "svga.h"
#include "gui.h"
#include "gui_defines.h"

void gui_draw_dialogue_frame(framebuffer_t* buffer, window_t* info);
void gui_draw_modal_frame(framebuffer_t* buffer, window_t* info);

static svga_mode_info_t* gui_screen_mode;

/*
 * Sets the current SVGA mode.
 */
void gui_set_screen_mode(svga_mode_info_t* mode) {
	gui_screen_mode = mode;
}

/*
 * Draws a window or dialogue frame.
 */
void gui_draw_window_frame(framebuffer_t* buffer, window_t* info) {
	if(info->isDialogue) {
		gui_draw_dialogue_frame(buffer, info);
	} else {
		gui_draw_modal_frame(buffer, info);
	}
}

/*
 * Draws a dialogue's frame.
 */
void gui_draw_dialogue_frame(framebuffer_t* buffer, window_t* info) {
	if(info->isActive) {
		gui_fill_rect(buffer, info->position, SVGA_24TO16BPP(GUI_WINDOW_BG));
	} else {
		gui_fill_rect(buffer, info->position, SVGA_24TO16BPP(GUI_WINDOW_BG));
	}
}

/*
 * Draws a modal window's frame.
 */
void gui_draw_modal_frame(framebuffer_t* buffer, window_t* info) {
	if(info->isActive) {
		gui_fill_rect(buffer, info->position, SVGA_24TO16BPP(GUI_WINDOW_BG));
	} else {
		gui_fill_rect(buffer, info->position, SVGA_24TO16BPP(GUI_WINDOW_BG));
	}
}

/*
 * Fills a rect with a specific colour.
 */
void gui_fill_rect(framebuffer_t* buffer, rect_t rect, uint16_t colour) {
	uint16_t *writePtr = buffer->ptr;

	uint16_t width = (buffer->size.width < rect.pt.x + rect.size.width) ? buffer->size.width-rect.pt.x : rect.size.width;
	uint16_t height = (buffer->size.height < rect.pt.y + rect.size.height) ? buffer->size.height-rect.pt.y : rect.size.height;

	writePtr += rect.pt.y * buffer->size.height;

	for(int y = 0; y < height; y++) {
		for(int x = rect.pt.x; x < rect.pt.x+width; x++) {
			writePtr[x] = colour;
		}

		writePtr += buffer->size.width;
	}
}

/*
 * Renders a bitmap on the screen.
 */
void gui_draw_bmp(void* bitmap, int px, int py) {
	void *buf = (void *) 0xD0000000;

	uint8_t *read = (uint8_t *) bitmap;
	uint8_t *write = buf + (px * 4) + (py * 0x1000);

	int width, height;
	// Read width, height from file
	width = read[0x12] | (read[0x13] << 8);
	height = read[0x16] | (read[0x17] << 8);

	// Read pointer (bottom to top)
	read += read[0x0E] + 0x0E;
	read += (width * 3 * height) - (width * 3);

	for(int y = 0; y < height; y++) {
		uint8_t *ptr = write + (y * 0x1000);

		for(int x = 0; x < width; x++) {
			ptr[0] = read[0];
			ptr[1] = read[1];
			ptr[2] = read[2];

			read += 3;
			ptr += 4;
		}

		read -= (width * 6);
	}
}