#include <types.h>

void sys_pic_irq_set_mask(uint8_t IRQline);
void sys_pic_irq_clear_mask(uint8_t IRQline);
void sys_pic_irq_remap(uint8_t offset1, uint8_t offset2);
uint16_t sys_pic_irq_get_isr();
uint16_t sys_pic_irq_get_irr();
void sys_pic_irq_eoi(uint8_t irq);
void sys_pic_irq_disable();
