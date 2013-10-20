	BITS	16

; Size of second stage loader, in sectors
stage2_len:		EQU $2F
; Start of second stage loader on disk, in sectors
stage2_start:	EQU $02
; Segment to load second stage to (Physical address $00500)
stage2_seg:		EQU $0000
stage2_off:		EQU $0500

loader_start:
	mov		ax, $07C0											; Set up 4K stack space after this bootloader
	add 	ax, 288												; (4096 + 512) / 16 bytes per frame
	mov 	ss, ax
	mov 	sp, 4096											; Set up SP

	mov 	ax, $07C0											; Set data segment to where we're loaded
	mov 	ds, ax

	mov		BYTE [BootDevice], dl								; Save boot device number

	xor		ah, ah												; Change video mode (AH = $00)
	mov		al, $03												; 40x25 text mode
	int		$10													; Call video BIOS

	xor		di, di												; Clear colour value
	xor		dx, dx												; Row 0, col 0
	mov 	si, str_loadstage2									; Put string position into SI
	call 	print_string										; Call string printing routine

	; Enable the A20 gate so we can use the full address range
;	call	check_a20											; Determine if A20 gate is set
;	cmp		ax, $0												; Is A20 line disabled?
;	jne		.A20GateEnabled										; If not, branch.

	mov		ax, $2401											; Try to enable A20 using the BIOS
	int		$15
	cmp		ah, $0												; Was there an error?
	je		.A20GateEnabled										; If not, branch

	in		al, $92												; Use "Fast A20 Gate" method
	test	al, 2
	jnz		.A20GateEnabled
	or		al, 2
	and		al, $0FE
	out		$92, al

.A20GateEnabled:
	xor		ah, ah												; Reset drive controller
	mov		dl, BYTE [BootDevice]								; Device number
	int		$13													; Reset drive

.readStage2:
	mov 	si, ExtendedRead_Table								; address of "disk address packet"
	mov 	ah, $42												; Extended Read
	mov		dl, BYTE [BootDevice]								; Device number
	int 	$13
	jc 		SHORT .readErr

	mov		dl, BYTE [BootDevice]								; Fetch boot device
	jmp		stage2_seg:stage2_off								; Jump to second stage

.readErr:
	mov		dx, WORD [LastCursorPosition]						; Read last cursor position
	mov		di, $04F											; White text on red background
	mov 	si, str_floppyError									; Put string position into SI
	call 	print_string										; Call string printing routine

	xor		ax, ax												; Clear AX
	int		$16													; Wait for keypress

	jmp		.readStage2

;========================================================================================
; Outputs the string in SI to the VGA adapter in text mode using INT10h.
; Note that the start position of the string on-screen (row, col) is in EDX.
;========================================================================================
print_string:
	push	dx													; Push column

	test	di, di												; Check if DI is set
	jz		.useDefaultColour									; If so, branch

	mov		ax, di												; Set colour
	mov		bl, al												; Get low byte only
	jmp		SHORT .setCursor

.useDefaultColour:
	mov		bl, $007											; Light gray text on black background

.setCursor:
	mov 	ah, $02												; Set cursor position
	xor		bh, bh												; Video page 0 (BH = 0)
	int		$10													; Set cursor

.repeat:
	lodsb														; Get character from string
	cmp 	al, 0
	je		.done												; If char is zero, end of string
	
	cmp 	al, $0A												; Process newline
	je		.newline

	mov		cx, $01												; Write one ASCII character
	xor		bh, bh												; Video page 0 (BH = 0)
	mov 	ah, $09												; Write character
	int		$10													; Print character

	mov 	ah, $02												; Set cursor position
	xor		bh, bh												; Video page 0 (BH = 0)
	inc		dl													; Increment column
	int		$10													; Set cursor

	jmp		.repeat

.done:
	mov		WORD [LastCursorPosition], dx						; Write last cursor position
	pop		dx													; Pop position
	xor		di, di												; Clear colour
	ret

.newline:
	pop		dx													; Get original column
	inc		dh													; Increment row
	push	dx													; Push it back to stack
	jmp		.repeat


;========================================================================================
; Disk timestamp
;========================================================================================
	times	0xDA-($-$$) db 0									; Align at 0xDA

	dw		0
	db		$80													; Original physical drive
	db		$00													; Secs
	db		$00													; Mins
	db		$00													; Hours

	times	0xE0-($-$$) db 0									; Align at 0xE0

;========================================================================================
; Checks if the A20 gate is enabled.
;
; Returns:  0 in ax if the a20 line is disabled (memory wraps around)
;			1 in ax if the a20 line is enabled (memory does not wrap around)
;========================================================================================
check_a20:
	pushf
	cli
 
	xor		ax, ax ; ax = 0
	mov		es, ax
 
	not		ax ; ax = 0xFFFF
	mov		ds, ax
 
	mov		di, 0x0500
	mov		si, 0x0510
 
	mov		al, BYTE [es:di]
	push	ax
 
	mov		al, BYTE [ds:si]
	push	ax
 
	mov		BYTE [es:di], 0x00
	mov		BYTE [ds:si], 0xFF
 
	cmp		BYTE [es:di], 0xFF
 
	pop		ax
	mov		BYTE [ds:si], al
 
	pop		ax
	mov		BYTE [es:di], al
 
	xor		ax, ax
	je		.done
 
	mov		ax, 1
 
.done:
	popf
	ret

;========================================================================================
; DATA SECTION
;========================================================================================
str_loadstage2:
	db 		"Loading Stage 2", $0A, 0

str_floppyError:
	db 		"Read error, press key to retry", $0A, 0

BootDevice:
	db		$80

LastCursorPosition:
	dw		0

	align 2
ExtendedRead_Table:
	db	$10
	db	0
	dw	stage2_len											; Num blocks
	dw	stage2_off											; Dest
	dw	stage2_seg											; Memory page
	dd	stage2_start-1										; Starting LBA
	dd	0													; more storage bytes only for big lba's ( > 4 bytes )

;========================================================================================
; MBR part 2
;========================================================================================
	times	0x1B8-($-$$) db 0									; Align at 0x1B8

	dd		0													; Disk signature
	dw		0													; Reserved

	; FAT partition 0
	db		$80													; Bootable
	db		$0FE, $0FF, $0FF									; CHS start (1023, 254, 63)
	db		$0C													; FAT32 with LBA addressing
	db		$0FE, $0FF, $0FF									; CHS end (1023, 254, 63)
	dd		$40													; FAT partition table starts at sector $40 (32K of padding for bootloader)
	dd		$2FB24												; 100 MB
	; End FAT partition 0

	dd		0, 0, 0, 0											; Partition entry 2
	dd		0, 0, 0, 0											; Partition entry 3
	dd		0, 0, 0, 0											; Partition entry 4

	times	510-($-$$) db 0										; Pad remainder of boot sector with 0s
	dw 		0xAA55												; Standard PC boot signature