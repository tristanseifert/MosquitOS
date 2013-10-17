#include <types.h>

void sys_pit_init(uint8_t channel, uint8_t mode);
void sys_pit_set_reload(uint8_t ch, uint16_t count);