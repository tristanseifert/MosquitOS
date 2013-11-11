#ifndef GUI_H
#define GUI_H

#include <types.h>

/*
 * GUI Library
 *
 * All functions take a pointer to a 16bpp memory buffer and read/write to that
 * buffer. This buffer may be a real framebuffer, memory backbuffer, or a some
 * other mapped memory, so long as it can be written and read without any
 * restrictions.
 *
 * Note that coordinates are relative to the buffer, and requesting to a draw
 * operation outside the backbuffer's rect will result in clipping.
 */

typedef struct frame {
	uint32_t width;
	uint32_t height;
} frame_t;

typedef struct point {
	uint32_t x;
	uint32_t y;
} point_t;

typedef struct rect {
	point_t pt;
	frame_t size;
} rect_t;

typedef struct window {
	rect_t position;
	char* title;

	bool isDialogue;
	bool isActive;
} window_t;

typedef struct framebuffer {
	uint16_t *ptr;
	frame_t size;
} framebuffer_t;

void gui_fill_rect(framebuffer_t* buffer, rect_t rect, uint16_t colour);

void gui_draw_window_frame(framebuffer_t* buffer, window_t* info);
void gui_set_screen_mode(svga_mode_info_t* mode);

#endif