BITS 16
section .text 
GLOBAL v86_trampoline
GLOBAL v86_trampoline_end

; This is the trampoline we map into the Virtual 8086 process memory space,
; and begin execution in. It will handle all the neccesary system setup and
; cleanup, as well as jumping to custom code we write.

v86_trampoline:
	; Set up code and data segments
	xor		ax, ax
	mov		cs, ax
	mov		ds, ax

	; Set up stack
	mov		ax, 0x8000
	mov		ss, ax
	mov		sp, 0xF000

	; Call user function
	call	0x0100:0x0000
	nop

	; Exit V86 mode
	int		0xF8

v86_trampoline_end:

BITS 32