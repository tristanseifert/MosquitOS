/*
 * Dummy IRQ handler
 */
.globl	_sys_dummy_irq
.align	4

_sys_dummy_irq:
	iret

/*
 * Timer tick handler
 */
.globl	_sys_timer_tick_irq
.align	4

_sys_timer_tick_irq:
	pushal
	call	_sys_timer_tick_handler
	popal
	iret
