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