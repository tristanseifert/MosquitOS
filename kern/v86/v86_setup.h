#ifndef V86_SETUP_H
#define V86_SETUP_H

#include <types.h>

typedef struct {
	uint16_t segment;
	uint16_t offset;
} __attribute__((packed)) v86_address_t;

int v86_init();

#endif