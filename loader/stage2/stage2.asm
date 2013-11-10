	BITS	16
	org		$0500

; Kernel will be loaded to segment $0A80, or $00A800 physical
kern_loc:				EQU $0A80
kern_loc_phys:			EQU kern_loc<<4
kern_start:				EQU 6
kern_len:				EQU 32									; Length in sectors

; Location to store various BIOS info at
Kern_Info_Struct:		EQU $0160								; $001600 phys (len = $400 max)
Kern_Info_StructPhys:	EQU (Kern_Info_Struct<<4)
VESA_SupportedModes:	EQU	$01A0								; $001A00 phys (len = $200)
BIOS_MemMapSeg:			EQU	$0200								; $002000 phys (len = $800 max)

; Physical protected mode addresses
MMU_PageDir:			EQU $003000
MMU_PageTable1:			EQU $004000
MMU_PageTable2:			EQU $005000
MMU_PageTable3:			EQU $006000
MMU_PageTable4:			EQU $007000
MMU_PageTable5:			EQU $008000

;	+$00 uint32_t munchieValue; // Should be "KERN"
;	+$04 uint16_t supportBits;
;	+$06 uint16_t high16Mem; // 64K blocks above 16M
;	+$08 uint16_t low16Mem; // 1k blocks below 16M
;	+$0A uint32_t memMap; // 32-bit ptr to list
;	+$0E uint16_t numMemMapEnt; // Number of entries in above map
;	+$10 uint8_t vesaSupport;
;	+$11 uint8_t bootDrive;
;	+$12 uint32_t vesaMap;

stage2_start:
	mov		ax, $8000											; AX = stack segment value (Stack to go at $80000)
	mov 	ss, ax
	mov 	sp, 4096											; Set up SP

	mov 	ax, cs												; Set data segment to where we're loaded
	mov 	ds, ax

	mov		BYTE [BootDevice], dl								; Save boot device number
	mov		BYTE [FAT_Drive], dl								; Set FAT read drive

	mov		DWORD [Kern_Info_StructPhys], "KERN"				; Set magic value for kern struct

	mov		al, [BootDevice]									; Set boot drive
	mov		BYTE [Kern_Info_StructPhys+$11], al					; ""

	; Set up video
	mov		ah, $00												; Change video mode
	mov		al, $03												; 80x25 text mode
	int		$10													; Call video BIOS

	mov 	si, str_stage2loaded								; Put string position into SI
	xor		dx, dx												; Cursor position
	mov		di, $2F												; Set colour
	call 	print_string										; Call string printing routine

	; Call VESA BIOS routines to get supported video modes
	mov		ax, VESA_SupportedModes								; Memory location of supported mode struct
	mov		es, ax												; ""
	xor		di, di												; Offset 0 in segment

	mov		[es:di], DWORD "VBE2"								; Tell BIOS we want 512 bytes of data

	mov		ax, $4F00											; VESA BIOS routines — get supported modes
	int		$10													; Perform lookup

	test	ah, ah												; Is AH not zero (i.e. error)
	je		.vesaDone											; If so, VESA is unsupported

.vesaDone:
	; Call BIOS to get memory information
	xor 	cx, cx
	xor 	dx, dx
	mov 	ax, $0E801
	int 	$15													; Request upper memory size
	jc 		error_memoryDetect
	cmp 	ah, $86												; Unsupported function
	je		error_memoryDetect
	cmp		ah, $80												; Invalid command
	je		error_memoryDetect
	jcxz 	.useax												; Was the CX result invalid?
 
	mov		ax, cx												; Number of continuous 1K blocks (1M-16M)
	mov		bx, dx												; Number of continuous 64K block above 16M

.useax:
	xor		di, di												; Clear DI
	mov		WORD [MemBlocksAbove16M], bx						; Store amount of memory available
	mov		WORD [Kern_Info_StructPhys+$06], bx					; Highmem
	mov		WORD [MemBlocksBelow16M], ax						; ""
	mov		WORD [Kern_Info_StructPhys+$08], ax					; Lowmem

	call	display_memsize										; Display the memory size

	; Fetch memory map
	mov 	ax, BIOS_MemMapSeg									; Write mem map to $01800 in physical space
	mov 	es, ax
	xor		di, di												; Start of segment

	call	fetch_mem_map										; Fetch a memory map
	jc 		SHORT error_memoryDetect							; Branch if error

	mov		WORD [Kern_Info_StructPhys+$0E], bp					; ""
	mov		DWORD [Kern_Info_StructPhys+$0A], (BIOS_MemMapSeg<<4); Physical location of table

	; Initialise FAT library
	call	FAT_Init

	; Check which partitions are bootable from MBR partition map
	call	find_bootable_partitions
	mov		BYTE [HDD_Selected], 0								; Clear HDD selection

	; Set up the partition chooser UI
	call	render_partition_chooser

	; Process keypresses, and loads kernel from FS if ENTER is pressed
	call	chooser_loop
	jmp		boot

;========================================================================================
; Memory detection error handler
;========================================================================================
error_memoryDetect:
	mov 	si, str_errorDetectMem								; Put string position into SI
	call 	print_error											; Call string printing routine
	jmp		$

;========================================================================================
; Code to boot the kernel
;========================================================================================
boot:
	; Hide cursor
	xor		dx, dx												; Clear dx
	not		dx													; dx = $FFFF
	mov 	ah, $02												; Set cursor position
	xor		bh, bh												; Video page 0 (BH = 0)
	int		$10													; Set cursor

	; Go into SVGA mode $101 (640x480x8bpp)
	mov		bx, $8101											; SVGA mode
	mov		ax, $4F02											; SVGA routine calls
	int		$10													; Call video BIOS

;	mov		ax, $0013
;	int		$10


	; Set up GDT
	cli															; Disable ints
	lgdt	[gdt_table]											; Set up GDTR

	; Jump into protected mode, woot!
	mov		eax, cr0											; Get control reg
	or		al, 00000001b										; Set PE bit
	mov		cr0, eax											; Write control reg

	; Set up selectors
	mov		ax, $10												; DATA32_DESCRIPTOR
	mov		ds, ax												; Set data selector

	mov		ax, $10												; DATA32_DESCRIPTOR
	mov		es, ax												; Update other selectors to point to data segment
	mov		fs, ax
	mov		gs, ax
	mov		ss, ax

	; The kernel is loaded to $00003000 phys (segmented address 0300h:0000h)
	db		$66													; 32-bit prefix
	db		$0EA												; Far jump opcode
	dd		copy_kernel											; Jump to kernel copying routine
	dw		$08													; Selector for CODE32_DESCRIPTOR

	BITS	32
copy_kernel:
	mov		esp, $400000										; Stackzors at $400000

	mov		eax, kern_loc_phys									; Physical kernel location
	mov		ebx, $00100000										; Destination memory address
	mov		ecx, $4000											; Number of long-words to copy (64KB)

	align	4													; DWORD align
.copy:
	mov		edx, DWORD [eax]									; Read a DWORD from lowmem
	mov		DWORD [ebx], edx									; Write DWORD to himem

	add		eax, $04											; Increment read ptr
	add		ebx, $04											; Increment write ptr

	loop	.copy												; Loop and copy everything

	; Here, we build a page directory and table to map $C0000000 to $00100000.
	xor		eax, eax
	mov		ebx, MMU_PageDir
	mov		ecx, $1000

.clrTablesLoop:
	mov		DWORD [ebx], eax
	add		ebx, $04
	loop	.clrTablesLoop


	; Since we only need to map 4M for right now, concern ourselves only with entry 0x300 and 0x000
	; Also, map 0x00000000 to 0x003FFFFF
	mov		DWORD [MMU_PageDir+0x000], (MMU_PageTable1 | $3)

	mov		DWORD [MMU_PageDir+0xC00], (MMU_PageTable2 | $3)
	mov		DWORD [MMU_PageDir+0xC04], (MMU_PageTable3 | $3)
	mov		DWORD [MMU_PageDir+0xC08], (MMU_PageTable4 | $3)
	mov		DWORD [MMU_PageDir+0xC0C], (MMU_PageTable5 | $3)


	; Run a loop 1024 times to fill the first page table
	mov		ecx, $400
	xor		ebx, ebx											; Page table offset
	mov		eax, DWORD $00000007									; Physical address start

.fillPageTable1:
	mov		DWORD [MMU_PageTable1+ebx*4], eax					; Write physical location

	inc		ebx													; Go to next entry in page table
	add		eax, $1000											; Increment physical address
	loop	.fillPageTable1


	; Run a loop 8192 times to fill the second page table
	mov		ecx, $1000
	xor		ebx, ebx											; Page table offset
	mov		eax, DWORD $00100007								; Physical address start

.fillPageTable2:
	mov		DWORD [MMU_PageTable2+ebx*4], eax					; Write physical location

	inc		ebx													; Go to next entry in page table
	add		eax, $1000											; Increment physical address
	loop	.fillPageTable2


	; Set paging directory to CR3
	mov		eax, MMU_PageDir
	mov		cr3, eax

	; Enable paging in CR0
	mov		eax, cr0
	or		eax, $80000000
	mov		cr0, eax

	; Jump into kernel
	jmp		$0C0000000

	BITS	16

;========================================================================================
; Renders the partition chooser
;========================================================================================
render_partition_chooser:
	mov 	si, str_select_partition							; Put string position into SI
	mov		dx, $0501											; Cursor position
	mov		di, $07												; Set colour
	call 	print_string										; Call string printing routine

	mov		WORD [LastCursorPosition], 0x0704					; Read last cursor position

	mov		edx, HDD_BootablePartitions							; EDX contains bootable partition ptr
	mov		cx, $04												; Loop 4x

.disp_loop:
	mov		al, BYTE [edx]										; Read bootability
	and		al, $80												; Get high bit only
	cmp		al, $80												; Is it $80?
	jne		.not_bootable										; If not, it's not a bootable drive

	call	.render_boot										; Render bootable drive label

	jmp		SHORT .next											; Skip over non-bootable code

.not_bootable:
	call	.render_noboot										; Render non-bootable drive label

.next:
	inc		edx													; Go to next item
	loop	.disp_loop											; Loop over all 4 partitions

	mov		dx, $0C01
	mov		si, str_err_clear_err								; Clear error
	call	print_string										; Display

	ret

;========================================================================================
; Renders an entry for a bootable drive
;========================================================================================
.render_boot:
	mov		al, $04												; Max drive num to al
	sub		al, cl												; Subtract loop counter
	mov		BYTE [.index], al									; Write index
	add		al, $30												; ASCII numbers

	mov		DWORD [Temp_StrBuf], "hd0,"							; "hd0," text
	mov		BYTE [Temp_StrBuf+4], al							; Drive number converted to ascii
	mov		WORD [Temp_StrBuf+5], ": "							; Colon, space

	pusha														; Push registers
	mov		edx, Temp_StrBuf+7									; String buffer write place
	mov		ebx, HDD_PartitionNames								; Partition names
	mov		cx, $0C												; $0C characters

.copyNameLoop:
	mov		al, BYTE [ebx]										; Copy a character
	mov		BYTE [edx], al										; Write to temp buffer
	inc		ebx													; Increment read pointer
	inc		edx													; Increment write pointer
	loop	.copyNameLoop

	mov		WORD [edx-1], 0x000A								; Insert newline

	xor		eax, eax											; Clear EAX
	mov		edx, HDD_BootablePartitionsFATType					; FAT type matrix
	add		dl, BYTE [.index]
	mov		al, BYTE [edx]										; Read FAT type to AL

	mov		bl, BYTE [HDD_Selected]								; Read index of selected HDD
	and		bl, $3												; Get low 2 bits only
	cmp		cl, bl												; Is current drive equal to selection?
	jne		.no_highlight										; If not, branch.

	mov		di, $070											; Black text on white background

.no_highlight:
	mov		dx, WORD [LastCursorPosition]						; Read last cursor position
	mov		si, Temp_StrBuf										; Temporary string buffer
	call 	print_string										; Call string printing routine
	popa														; Pop registers

	ret

.index:
	db	0

;========================================================================================
; Renders an entry for a non-bootable drive.
;========================================================================================
	align 4

.render_noboot:
	mov		al, $04												; Max drive num to al
	sub		al, cl												; Subtract loop counter
	add		al, $30												; ASCII numbers

	mov		DWORD [Temp_StrBuf], "hd0,"							; "hd0," text
	mov		BYTE [Temp_StrBuf+4], al							; Drive number converted to ascii
	mov		DWORD [Temp_StrBuf+5], ": No"						; "Not Bootable"
	mov		DWORD [Temp_StrBuf+9], "t Bo"
	mov		DWORD [Temp_StrBuf+13], "otab"
	mov		WORD [Temp_StrBuf+17], "le"
	mov		WORD [Temp_StrBuf+19], 0x000A						; Newline, terminator

	pusha														; Push registers

	mov		bl, BYTE [HDD_Selected]								; Read index of selected HDD
	and		bl, $3												; Get low 2 bits only
	cmp		cl, bl												; Is current drive equal to selection?
	jne		.no_highlight2										; If not, branch.

	mov		di, $070											; Black text on white background

.no_highlight2:
	mov		dx, WORD [LastCursorPosition]						; Read last cursor position
	mov		si, Temp_StrBuf										; Temporary string buffer
	call 	print_string										; Call string printing routine
	popa														; Pop registers

	ret

;========================================================================================
; Handle keypresses for chooser
;========================================================================================
chooser_loop:
	jmp		partition_chooser_enter

	xor		ah, ah												; Wait for keystroke
	int		$16													; Call into BIOS

	cmp		ah, $50												; Down pressed?
	je		partition_chooser_dn

	cmp		ah, $48												; Up pressed?
	je		partition_chooser_up

	cmp		ah, $1C												; Enter pressed?
	je		partition_chooser_enter

	jmp		chooser_loop

partition_chooser_enter:
	xor		bx, bx												; Clear BX
	mov		bl, BYTE [HDD_Selected]								; Get selection

	mov		al, BYTE [HDD_BootablePartitions+bx]				; Check bootability status
	and		al, $80												; Get high bit only
	cmp		al, $80
	jne		.noBootErr											; If not bootable, branch


	mov		si, kernel_filename									; Filename to find
	call	FAT_FindFileAtRoot									; Find file
	jc		.fileNotFound										; Carry set = KERNEL.BIN not found

	mov		DWORD [kernel_cluster], eax							; Store cluster

	xor		ax, ax												; Segment 0
	mov		es, ax												; Write segment											
	mov		di, kern_loc_phys									; Offset into segment

	mov		eax, DWORD [kernel_cluster]							; Kernel's cluster location
	call	FAT_ReadFile										; Read file

	mov 	si, str_kernel_loaded_ok							; Put string position into SI
	mov		dx, $0C01											; Cursor position
	mov		di, $02												; Set colour
	call 	print_string										; Call string printing routine

	ret

.noBootErr:
	mov		dx, $0C01
	mov		si, str_err_not_bootable							; Not bootable error
	call	print_error											; Display
	jmp		chooser_loop

.fileNotFound:
	mov		dx, $0C01
	mov		si, str_err_kern_not_found							; Not found error
	call	print_error											; Display
	jmp		chooser_loop

partition_chooser_dn:
	mov		al, BYTE [HDD_Selected]								; Read selection
	dec		al													; Move cursor up
	and		al, $03												; Get low 2 bits only
	mov		BYTE [HDD_Selected], al								; Restore

	call	render_partition_chooser							; Update display
	jmp		chooser_loop

partition_chooser_up:
	mov		al, BYTE [HDD_Selected]								; Read selection
	inc		al													; Move cursor down
	and		al, $03												; Get low 2 bits only
	mov		BYTE [HDD_Selected], al								; Restore

	call	render_partition_chooser							; Update display
	jmp		chooser_loop

;========================================================================================
; Finds all partitions that are bootable.
;========================================================================================
find_bootable_partitions:
	mov		cx, $4												; MBR contains 4 partition maps

	mov		ax, $07C0											; Bootloader at 0x7C00
	mov		es, ax												; Set ES to the bootloader's place in memory
	mov		di, $1BE											; Start of partition map

	mov		ax, ds												; Fetch data segment
	mov		gs, ax												; Set GS to data segment

	mov		esi, HDD_BootablePartitions							; ESI contains bootable partition ptr
	mov		edx, HDD_PartitionNames								; Partition name ptr

.loop:
	mov		al, BYTE [es:di]									; Read bootable flag
	and		al, $80												; Get high bit only
	cmp		al, $80												; Is it $80?
	jne		.not_bootable										; If not, it's not a bootable drive

	mov		BYTE [esi], al										; Write bootability flag

	mov		BYTE [HDD_Selected], cl								; Write index

	; Try to read the LBA of the partition
	mov		eax, DWORD [es:di+8]								; Read partition LBA
	cmp		eax, $00											; Is it zero?
	je		.no_valid_lba										; If so, fuck off

	mov		DWORD [ExtendedRead_Table+0x08], eax				; Write LBA
	mov		WORD [ExtendedRead_Table+0x02], 0x01				; Read one sector
	mov		WORD [ExtendedRead_Table+0x04], SectorBuf			; Temporary sector buffer offset (seg 0)

	mov		DWORD [FAT_PartitionOffset], eax					; Write offset into FAT

	pusha														; Push registers (BIOS may clobber them)
	mov 	si, ExtendedRead_Table								; address of "disk address packet"
	mov 	ah, $42												; Extended Read
	mov		dl, BYTE [BootDevice]								; Device number
	int 	$13
	popa														; Pop registers
	jc 		SHORT .no_valid_lba									; If error, fuck off

	call	.typeDetermine										; Determine type and label loc

	push	cx													; Back up original loop counter

	mov		cx, $0B												; Copy 0xB bytes
.copy_str_loop:
	mov		bl, BYTE [eax]										; Copy from source
	mov		BYTE [edx], bl										; Write to target buffer
	inc		eax													; Increment read pointer
	inc		edx													; Increment write pointer
	loop	.copy_str_loop										; Copy all bytes.

	pop		cx													; Restore original loop counter.

.no_valid_lba:

.not_bootable:
	add		di, $10												; Go to next entry in bootsector
	inc		esi													; Write next bootability flag
	add		edx, $0C											; Each entry of partition names is 0x0C in length
	loop	.loop												; Loop through all partitions

.done:
	ret

; Determines FAT type and stores pointer to read volume label in eax
.typeDetermine:
	push	esi													; Push old ESI
	mov		si, SectorBuf										; Sector buffer
	call	FAT_DetermineType									; Determine type of FS
	pop		esi													; Pop ESI

	push	edx													; Back up EDX
	mov		bl, $04												; Max drive num to al
	sub		bl, cl												; Subtract loop counter

	mov		edx, HDD_BootablePartitionsFATType					; FAT type ptr
	sub		dl, bl												; Subtract index
	mov		BYTE [edx], al										; Write FAT size
	pop		edx													; Restore EDX

	cmp		al, $20												; Is it a FAT32 volume?
	jne		.fat16_label										; If so, branch

	mov		eax, SectorBuf+$47									; FAT32 has volume label at 0x47

	jmp		.copy												; Copy label

	; Extract volume label from sector buffer
.fat16_label:
	mov		eax, SectorBuf+$2B									; FAT16 has volume label at 0x2B

.copy:
	ret

.index:
	db		0

;========================================================================================
; Displays the memory size on the screen 
;========================================================================================
display_memsize:
	mov 	si, str_available_lomem								; Put string position into SI
	mov		dx, $0200
	call 	print_string										; Call string printing routine
	mov		ax, WORD [MemBlocksBelow16M]						; Get total of memory blocks to EDX
	call	hex_to_ascii

	mov 	si, str_available_himem								; Put string position into SI
	mov		dx, $0300
	call 	print_string										; Call string printing routine
	mov		ax, WORD [MemBlocksAbove16M]						; Get total of memory blocks to EDX
	call	hex_to_ascii

	ret

;========================================================================================
; Outputs the string in SI to the VGA adapter in text mode using INT10h with the styling
; required for an error string.
; Note that the start position of the string on-screen (row, col) is in EDX.
;========================================================================================
print_error:
	mov		di, $04F											; White text on red background
	jmp 	print_string										; Call string printing routine

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
; Prints the character in al to the screen at the current cursor position, using the
; colour in di.
;========================================================================================
putc:
	test	di, di												; Check if DI is set
	jz		.useDefaultColour									; If so, branch

	mov		ax, di												; Set colour
	mov		bl, al												; Get low byte only
	jmp		SHORT .setCursor

.useDefaultColour:
	mov		bl, $007											; Light gray text on black background

.setCursor:
	mov		dx, WORD [LastCursorPosition]						; Read last cursor position
	mov 	ah, $02												; Set cursor position
	mov		bh, $0												; Video page 0
	int		$10													; Set cursor

	mov		cx, $01												; Write one ASCII character
	mov		bh, $0												; Video page 0
	mov 	ah, $09												; Write character
	int		$10													; Print character

	inc		dl													; Increment column

	mov 	ah, $02												; Set cursor position
	mov		bh, $0												; Video page 0
	int		$10													; Set cursor

	mov		WORD [LastCursorPosition], dx						; Write last cursor position
	xor		di, di												; Clear colour
	ret

;========================================================================================
; Prints the value in eax to the screen.
;========================================================================================
hex_to_ascii:
	xor 	cx, cx

	mov 	cl, ah												; Move high byte of ax to cl
	call 	.nibble_high										; Print low nibble to ASCII
	mov 	cl, ah
	call 	.nibble_low
	mov 	cl, al
	call 	.nibble_high
	mov 	cl, al
	call 	.nibble_low
	ret

.nibble_high:
	shr 	cl, $04
	jmp 	.convert_check

.nibble_low:
	and 	cl, $0F
	jmp 	.convert_check

.convert_check:
	cmp 	cl, $0A
	jge 	.letter
	add 	cl, $30
	push 	ax
	mov 	al, cl
	call 	putc
	pop 	ax
	ret

.letter:
	add 	cl, $37
	push	ax
	mov 	al, cl
	call	putc
	pop 	ax
	ret

;========================================================================================
; Uses BIOS INT $15, EAX $E820 function to get the memory map of the system
; input: 	es:di = destination buffer for 24 byte entries
; output: 	bp = entry count, trashes all registers except esi
;========================================================================================
fetch_mem_map:
	xor		ebx, ebx											; Clear EBX
	xor		bp, bp												; Use BP as an entry count
	mov		edx, $0534D4150										; Place "SMAP" into edx (magic value)
	mov		eax, $0E820											; Function call

	mov		[es:di+20], dword 1									; Write to the array so we have a valid ACPI 3.x entry
	mov		ecx, 24												; Ask BIOS for 24 bytes of data
	int		$15

	jc		SHORT .error										; If carry set, the function is unsupported

	mov		edx, $0534D4150										; Restore EDX in case trashed by BIOS
	cmp		eax, edx											; On success, EAX = "SMAP"
	jne		SHORT .error

	test	ebx, ebx											; ebx = 0 implies list is only 1 entry long (worthless)
	je		SHORT .error

	jmp		SHORT .startLoop									; Jump into the loop

.getEntryLoop:
	mov		eax, $0E820											; Reset command (EAX, ECX are trashed)
	mov		[es:di+20], dword 1									; Write to the array so we have a valid ACPI 3.x entry
	mov		ecx, 24												; Ask BIOS for 24 bytes of data
	int		$15

	jc		SHORT .done											; If carry set, we are done
	mov		edx, $0534D4150										; Restore EDX in case trashed by BIOS

.startLoop:
	jcxz	.skipEntry											; Skip any 0 length entries

	cmp		cl, 20												; Did we get 24-byte ACPI 3.x data?
	jbe		SHORT .notext

	test	BYTE [es:di+20], 1									; If so, is the "ignore this data" bit clear?
	je		SHORT .skipEntry

.notext:
	mov		ecx, [es:di+8]										; get lower dword of memory region length
	or		ecx, [es:di+12]										; Check if zero (OR with upper dword)
	jz		.skipEntry											; If length qword is 0, skip entry

	inc		bp													; We got a good entry, increment count, go to next entry

	add		di, 24

.skipEntry:
	test	ebx, ebx											; If EBX = 0, then the BIOS has given us all entries
	jne		SHORT .getEntryLoop

.done:
	clc															; There is "jc" on end of list to this point, so the carry must be cleared
	ret

.error:
	stc															; Set carry if this BIOS sucks ass and doesn't support this
	ret

;========================================================================================
; Writes a register dump to the VGA hardware
;========================================================================================
VGA_MISC_WRITE		EQU	$3C2
VGA_SEQ_INDEX		EQU	$3C4
VGA_SEQ_DATA		EQU	$3C5
VGA_CRTC_INDEX		EQU	$3D4
VGA_CRTC_DATA		EQU	$3D5
VGA_INSTAT_READ		EQU	$3DA

NUM_SEQ_REGS		EQU	5
NUM_CRTC_REGS		EQU	25

write_regs:
	push 	si
	push 	dx
	push 	cx
	push 	ax
	cld

; write MISC register
	mov 	dx, VGA_MISC_WRITE
	lodsb
	out 	dx, al

; write SEQuencer registers
	mov 	cx, NUM_SEQ_REGS
	xor 	ah, ah

write_seq:
	mov 	dx, VGA_SEQ_INDEX
	mov 	al, ah
	out 	dx, al

	mov 	dx, VGA_SEQ_DATA
	lodsb
	out 	dx, al

	inc 	ah
	loop 	write_seq

; write CRTC registers
; Unlock CRTC registers: enable writes to CRTC regs 0-7
	mov 	dx, VGA_CRTC_INDEX
	mov 	al, 17
	out 	dx, al

	mov 	dx, VGA_CRTC_DATA
	in		al, dx
	and 	al, $7F
	out 	dx, al

; Unlock CRTC registers: enable access to vertical retrace regs
	mov 	dx, VGA_CRTC_INDEX
	mov 	al, 3
	out 	dx, al

	mov 	dx ,VGA_CRTC_DATA
	in		al, dx
	or		al, $80
	out		dx, al

; make sure CRTC registers remain unlocked
	mov		al,[si + 17]
	and 	al, $7F
	mov 	[si + 17],al

	mov 	al,[si + 3]
	or		al, $80
	mov 	[si + 3], al

; now, finally, write them
	mov 	cx, NUM_CRTC_REGS
	mov 	ah, 0

write_crtc:
	mov 	dx, VGA_CRTC_INDEX
	mov 	al, ah
	out 	dx, al

	mov 	dx, VGA_CRTC_DATA
	lodsb
	out 	dx, al

	inc 	ah
	loop write_crtc

	pop 	ax
	pop 	cx
	pop 	dx
	pop 	si
	ret

;	align	32
;fat_loader_entry:
;	incbin		"./loader_c.bin"

;========================================================================================
; DATA SECTION
;========================================================================================
regs_90x60:
; MISC
	db	0E7h
; SEQuencer
	db	03h, 01h, 03h, 00h, 02h
; CRTC
	db	6Bh, 59h,  5Ah, 82h, 60h,  8Dh, 0Bh,  3Eh,
	db	00h, 47h,  06h, 07h, 00h,  00h, 00h,  00h,
	db	0EAh, 0Ch, 0DFh, 2Dh, 08h, 0E8h, 05h, 0A3h,
	db 	0FFh
; GC (no)
; AC (no)

str_stage2loaded:
	db 	'Stage 2 Bootloader (boot2)', 0

str_errorDetectMem:
	db 	"Error detecting available memory, cannot continue", 0

str_floppyError:
	db 	"Floppy Error, press any key to retry", $0A, 0

str_available_lomem:
	db 	"Continuous 01K blocks below 0x01000000: 0x", 0

str_available_himem:
	db 	"Continuous 64K blocks above 0x01000000: 0x", 0

str_select_partition:
	db 	"Use the cursor to select the partition to boot from.", 0

str_err_not_bootable:
	db 	"This partition is not marked as bootable!", 0

str_err_kern_not_found:
	db	"Could not find KERNEL.BIN at the root of the drive!", 0

str_err_clear_err:
	times	0x40 db 0x20
	db	0


str_kernel_loading:
	db 	"Loading kernel: ", 0

str_kernel_loaded_ok:
	db 	"Kernel loaded. Transferring control now...", 0

str_err_loadkernel:
	db 	"Could not load kernel: Fuck you", 0

BootDevice:
	db	0

LastCursorPosition:
	dw	0

MemBlocksAbove16M:
	dw	0

MemBlocksBelow16M:
	dw	0

MemMap_NumEntries:
	dw	0

HDD_BootablePartitions:
	dd	0

HDD_BootablePartitionsFATType:
	dd	0

HDD_Selected:
	db	0

HDD_PartitionNames:
	times	(0xB+1)*4 db 0

Temp_StrBuf:
	times	0x20 db 0

	align 2
ExtendedRead_Table:
	db	$10
	db	0
	dw	0														; Num blocks
	dw	0														; Dest
	dw	0														; Memory page
	dd	0														; Starting LBA
	dd	0	

kernel_cluster:
	dd	0

kernel_filename:
	db	"KERNEL  BIN", 0

;========================================================================================
; Global Descriptor Table
;========================================================================================
	align	$10

gdt_start:
	dd	$00, $00												; Null Descriptor

	; Code segment
	dw	$0FFFF													; Limit 0:15 = $0FFFF
	dw	$0000													; Base 0:15 = $0000
	db	$00														; Base 16:23 = $00
	db	$9A														; Access byte: Present, ring 0, Exec, grow up, R/W
	db	$0CF													; 4K pages, 32-bit, limit 16:19 = $F
	db	$00														; Base 24:31 = $00

	; Data segment
	dw	$0FFFF													; Limit 0:15 = $0FFFF
	dw	$0000													; Base 0:15 = $0000
	db	$00														; Base 16:23 = $00
	db	$92														; Access byte: Present, ring 0, Not exec, grow up, R/W
	db	$0CF													; 4K pages, 32-bit, limit 16:19 = $F
	db	$00														; Base 24:31 = $00	

gdt_table:
	dw	(gdt_table-gdt_start)-1									; Length
	dd	gdt_start												; Physical address to GDT	

	align 4														; DWORD align
	%include	"./fat.asm"

SectorBuf:
	times	512 db 0