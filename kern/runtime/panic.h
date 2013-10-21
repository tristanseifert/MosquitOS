#ifndef PANIC_H
#define PANIC_H

void panic(char *message, char *file, uint32_t line);
void panic_assert(char *file, uint32_t line, char *desc);

#endif