#ifndef FB_CONSOLE_H
#define FB_CONSOLE_H

#define CHAR_HEIGHT 16
#define CHAR_WIDTH 8

void fb_console_init();
void fb_console_putchar(unsigned char c);
void fb_console_control(unsigned char c);

void fb_console_set_font(void* reg, void* bold);

#endif