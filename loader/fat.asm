;========================================================================================
; FAT Filesystem Library v 0.1
; By Tristan Seifert
;========================================================================================
; Equates
;========================================================================================

;========================================================================================
; Initialises the FAT filesystem library
;========================================================================================
FAT_Init:
	mov		cx, (1024/4)										; Clear 2 sectors worth
	mov		edx, FAT_ReadBuffer									; Pointer to buffer
	xor		eax, eax											; Clear value ($00000000)

.clearLoop:
	mov		DWORD [edx], eax									; Clear a DWORD
	add		edx, $04											; Increment pointer
	loop	.clearLoop											; Loop

.done
	ret

;========================================================================================
; Determines the type of FAT, give that the first logical sector of the partition is
; pointed to by SI, and returns the FAT bit size in AL.
;
; This is how Microsoft recommends we determine FAT type:
;
; RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytesPerSec - 1)) / BPB_BytesPerSec
; DataSectors = Total Sectors - (BPB_ReservedSectors + (BPB_NumFATs * FATSz) + RootDirSectors)
; Count of clusters = DataSectors/BPB_SectorsPerCluster 
;
; FAT12: Total clusters < 4085
; FAT16: Total clusters > 4085 && < 65525
; FAT32: Total clusters > 65525
;========================================================================================
FAT_DetermineType:
	pusha														; Push registers

	call	FAT_ReadBPB											; 

	; Calculate RootDirSectors
	xor		eax, eax											; Clear EAX
	xor		ebx, ebx											; Clear EBX
	mov		bx,	WORD [FAT_BPB_RootEntCnt]						; Read root entries 
	shl		ebx, 5												; Each root entry is 32 bytes

	mov		ax, WORD [FAT_BPB_BytesPerSec]						; Read sector length
	dec		ax													; Subtract one
	add		eax, ebx											; Add root entry length to sector length

	xor		edx, edx											; Clear EDX
	mov		cx, WORD [FAT_BPB_BytesPerSec]						; Read bytes per sector
	div		cx													; Divide by sector length (result = ax)
	mov		WORD [FAT_RootDirSectors], ax						; Store result in memory

	; Calculate DataSectors
	xor		eax, eax											; Clear EAX
	mov		al, BYTE [FAT_BPB_NumFATs]							; Read number of FATs 

	mov		edx, DWORD [FAT_BPB_FATSz]							; Read FAT size
	mul		edx													; Multiply by number of FATs in eax

	xor		ecx, ecx											; Clear ECX
	mov		cx, WORD [FAT_BPB_ReservedSectors]					; Read number of reserved sectors

	add		eax, ecx											; Add count of reserved sectors to FAT size
	mov		ecx, DWORD [FAT_RootDirSectors]						; Read RootDirSectors
	add		eax, ecx											; Add root directory sectors

	xchg	eax, ecx											; Subtract all of the above from total sectors
	
	mov		eax, DWORD [FAT_BPB_TotSec]							; Read total sector count
	sub		eax, ecx											; Subtract from total sector count

	mov		DWORD [FAT_DataSectors], eax						; Store to memory

	; Calculate cluster count
	xor		ebx, ebx											; Clear EBX
	mov		bl, BYTE [FAT_BPB_SectorsPerCluster]				; Read sectors/cluster
	mov		eax, DWORD [FAT_DataSectors]						; Read number of data sectors
	div		ebx													; Divide by sectors/cluster value 

	mov		DWORD [FAT_TotalClusters], eax						; Store result in EAX

	; Now, do some comparisons!
	cmp		eax, 4085											; Is the FS FAT12?
	jg		.notFAT12											; If not, branch

	mov		BYTE [FAT_Type], 12									; FAT12
	jmp		SHORT .done											; Return

.notFAT12:
	cmp		eax, 65525											; Is the FS FAT16?
	jg		.notFAT16											; If not, branch

	mov		BYTE [FAT_Type], 16									; FAT16
	jmp		SHORT .done											; Return

.notFAT16:
	; Okay, if we get down here, it HAS to be FAT32 or a corrupt FS
	mov		BYTE [FAT_Type], 32									; FAT32

.done:
	popa														; Restore registers

	mov		al, BYTE [FAT_Type]									; Store FAT type in AL

	ret

;========================================================================================
; Reads the BPB from the FAT 1st sector in SI.
;========================================================================================
FAT_ReadBPB:
	mov		ax, WORD [si+11]									; Read Bytes/sector
	mov		WORD [FAT_BPB_BytesPerSec], ax						; ""

	mov		al, BYTE [si+13]									; Read sectors/cluster
	mov		BYTE [FAT_BPB_SectorsPerCluster], al				; ""

	mov		ax, WORD [si+14]									; Read reserved sectors
	mov		WORD [FAT_BPB_ReservedSectors], ax					; ""

	mov		al, BYTE [si+16]									; Read number of FATs
	mov		BYTE [FAT_BPB_NumFATs], al							; ""

	mov		ax, WORD [si+17]									; Read num root entries
	mov		WORD [FAT_BPB_RootEntCnt], ax						; ""

	xor		eax, eax											; Clear EAX

	mov		eax, DWORD [si+32]									; Read BPB_TotSec32 first
	cmp		eax, 0												; Is EAX zero?
	jne		.writeTotSec										; If not, branch

	mov		ax, WORD [si+19]									; Read BPB_TotSec16 first

.writeTotSec:
	mov		DWORD [FAT_BPB_TotSec], eax							; ""

	xor		eax, eax											; Clear EAX

	mov		ax, WORD [si+22]									; Read BPB_FATSz16
	cmp		ax, 0												; Is it zero?
	jne		.writeFATSz											; If not, branch.

	mov		eax, DWORD [si+36]									; Read BPB_FATSz32

.writeFATSz:
	mov		DWORD [FAT_BPB_FATSz], eax							; Write FAT size

	ret

;========================================================================================
; Data section
;========================================================================================
	align	4
FAT_BPB_BytesPerSec: ; file offset 11
	dw		0

FAT_BPB_SectorsPerCluster: ; file offset 13
	db		0

FAT_BPB_ReservedSectors: ; file offset 14
	dw		0

FAT_BPB_NumFATs: ; file offset 16
	db		0

FAT_BPB_RootEntCnt:	; file offset 17
	dw		0

; !!!
FAT_BPB_FATSz: ; file offset 22 if FAT16, 36 if FAT32 and word at 22 is 0
	dd		0

; Works with FAT32
; For the total sector count, try to read BPB_FATSz16 first (19), then the 32-bit at off 32
FAT_BPB_TotSec: ; file offset 32 if 19 = 0
	dd		0

; Calculated when identifying FS
FAT_RootDirSectors:
	dw		0

; Calculated when identifying FS
FAT_DataSectors:
	dd		0

; Calculated when identifying FS
FAT_TotalClusters:
	dd		0

FAT_Type:
	db		0

FAT_ReadBuffer:
