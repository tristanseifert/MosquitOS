#include "io_internal.h"
#include <unistd.h>

int putchar(char c) {
	// Set up a buffer
	char buffer[2];
	buffer[0] = c;
	buffer[1] = 0x00;

	if(fwrite(&buffer, 2, 1, STDOUT_FILENO) == 2) {
		return c;
	} else {
		return errno;
	}
}