#ifndef V86_SETUP_H
#define V86_SETUP_H

#include <types.h>

typedef struct {
	uint16_t segment;
	uint16_t offset;
} __attribute__((packed)) v86_address_t;

#define v86_segment_to_linear(x) ((x.segment<<4)+x.offset)

void v86_copyin(void* in_data, size_t num_bytes, v86_address_t address);
void* v86_copyout(v86_address_t address, void* out_data, size_t num_bytes);

#endif