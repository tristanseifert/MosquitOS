#ifndef VGA_CONSOLE_H
#define VGA_CONSOLE_H

void vga_console_init();
void vga_console_putchar(unsigned char c);
void vga_console_control(unsigned char c);

#endif