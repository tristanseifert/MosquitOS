;========================================================================================
; FAT Filesystem Library v 0.1
; By Tristan Seifert
;
; All sector values returned by functions are "logical," i.e. they are relative to the
; first sector of the filesystem.
;
; In addition, this library does not offer full support for FAT12 due to the uncommonality
; of it on media besides floppy disks.
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

.done:
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

	; Calculate number of data sectors
	xor		eax, eax											; Clear EAX
	mov		al, BYTE [FAT_BPB_NumFATs]							; Read number of FATs 

	mov		ecx, DWORD [FAT_BPB_FATSz]							; Read FAT size
	mul		ecx													; Multiply by number of FATs in eax

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

	call	FAT16_Calculate_RootDirSec							; Calculate location of root directory

	mov		BYTE [FAT_Type], 16									; FAT16
	jmp		SHORT .done											; Return

.notFAT16:
	; Okay, if we get down here, it HAS to be FAT32 or a corrupt FS
	mov		BYTE [FAT_Type], 32									; FAT32

.done:
	call	FAT_Calculate_Misc									; Calculate miscellaneous stuff
	popa														; Restore registers

	mov		al, BYTE [FAT_Type]									; Store FAT type in AL

	ret

;========================================================================================
; Calculates miscellaneous values that the FAT driver uses later
;========================================================================================
FAT_Calculate_Misc:
	xor		eax, eax											; Clear EAX
	xor		ebx, ebx											; Clear EBX
	xor		ecx, ecx											; Clear ECX

	mov		cx, WORD [FAT_RootDirSectors]						; Read number of root directory sectors
	mov		bx, WORD [FAT_BPB_ReservedSectors]					; Read reserved sectors to EBX

	add		ebx, ecx											; Add to root dir sectors reserved sectors

	xor		ecx, ecx											; Clear ECX
	mov		cl, BYTE [FAT_BPB_NumFATs]							; Read number of FATs to ECX 
	mov		eax, DWORD [FAT_BPB_FATSz]							; Read FAT size to EAX
	mul		ecx													; Multiply by number of FATs in ECX

	add		eax, ebx											; Add FAT sectors to root dir and reserved count

	mov		DWORD [FAT_FirstDataSector], eax					; Store first data sector

	xor		ecx, ecx											; Clear ECX
	mov		cx, WORD [FAT_RootDirSectors]						; Read number of root directory sectors
	sub		eax, ecx											; Subtract ecx
	mov		DWORD [FAT_FirstClusterLocation], eax				; Write shaften
	ret

;========================================================================================
; Calculates the sector for the root directory for FAT12 and FAT16.
;========================================================================================
FAT16_Calculate_RootDirSec:
	xor		eax, eax											; Clear EAX
	xor		ebx, ebx											; Clear EBX
	xor		ecx, ecx											; Clear ECX

	mov		bx, WORD [FAT_BPB_ReservedSectors]					; Read reserved sectors to EBX

	xor		eax, eax											; Clear EAX
	mov		al, BYTE [FAT_BPB_NumFATs]							; Read number of FATs 

	xor		ecx, ecx											; Clear ECX
	mov		cl, BYTE [FAT_BPB_NumFATs]							; Read number of FATs to ECX 
	mov		eax, DWORD [FAT_BPB_FATSz]							; Read FAT size to EAX
	mul		ecx													; Multiply by number of FATs in ECX

	add		eax, ebx											; Add reserved sector count

	mov		DWORD [FAT_BPB_RootClus], eax						; Write location of root cluster

.done:
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

	call	.readFAT32Specifics									; Read FAT32-specific data

.writeFATSz:
	mov		DWORD [FAT_BPB_FATSz], eax							; Write FAT size

	ret

; All FAT32-specific stuff is read here
.readFAT32Specifics:
	mov		bx, WORD [si+40]									; Read BPB_ExtFlags
	mov		WORD [FAT_BPB_ExtFlags], bx							; Write BPB_ExtFlags

	mov		bx, WORD [si+42]									; Read BPB_FSVer
	mov		WORD [FAT_BPB_FSVer], bx							; Write BPB_FSVer

	mov		ebx, DWORD [si+44]									; Read BPB_RootClus
	mov		DWORD [FAT_BPB_RootClus], ebx						; Write BPB_RootClus

	mov		bx, WORD [si+48]									; Read BPB_FSInfo
	mov		WORD [FAT_BPB_FSInfo], bx							; Write BPB_FSInfo


	ret

;========================================================================================
; Calculates the entry location for cluster N in the FAT.
; eax: Cluster number
; eax: Sector number containing the cluster
; ebx: Offset in sector
;========================================================================================
FAT_FindClusterInTable:
	pusha														; Back up regs

	xor		ebx, ebx											; Clear EBX
	xor		edx, edx											; Clear EDX

	mov		bl, BYTE [FAT_Type]									; Read FAT type
	cmp		bl, $20												; Is FAT32?
	je		.FAT32												; If so, jump

	shl		eax, 1												; Multiply cluster by 2
	jmp		.cont												; Skip over shift below

.FAT32:
	shl		eax, 2												; Multiply cluster by 4

.cont:
	; eax = Offset into FAT table

	xor		ecx, ecx											; Clear ECX
	mov		cx, WORD [FAT_BPB_BytesPerSec]						; Read bytes per sector
	div		ecx													; Divide offset by bytes/sector
	; eax = quotient, edx = remainder

	mov		bx, WORD [FAT_BPB_ReservedSectors]					; BX = reserved sector count
	add		eax, ebx											; Add to FAT offset (sectors)

	mov		DWORD [.secNum], eax								; Store sector number
	mov		WORD [.secOff], dx									; Store offset into sector

.done:
	popa														; Restore regs

	xor		ebx, ebx											; Clear EBX
	mov		eax, DWORD [.secNum]								; Get sector number
	mov		bx, WORD [.secOff]									; Get offset into sector

	ret

.secNum:
	dd		0

.secOff:
	dw		0

;========================================================================================
; Converts the cluster number in eax into a sector number.
;========================================================================================
FAT_ClusterToSector:
	push	ecx													; Back up EBX

	dec		eax													; Subtract 2 from cluster
	dec		eax													; ""

	xor		ecx, ecx											; Clear EBX
	mov		cl, BYTE [FAT_BPB_SectorsPerCluster]				; Read sectors/cluster
	mul		ecx													; Multiply by number of sectors per cluster

	xor		ecx, ecx											; Clear ECX
	mov		cx, WORD [FAT_RootDirSectors]						; Root directory sector
	add		eax, ecx											; Add to sector count

	mov		ecx, DWORD [FAT_FirstDataSector]					; Get first data sector
	add		eax, ecx											; Add data sector offset

	pop		ecx													; Restore EBX

	ret

;========================================================================================
; Reads the sector containing the FAT entry for the specified cluster, then returns the
; FAT read from the sector.
; eax: Cluster
; Sets carry flag if error.
;========================================================================================
FAT_ReadFAT:
	call	FAT_FindClusterInTable								; Locate cluster
	push	ebx													; Push offset into sector to stack

	; Read sector to memory
	call	FAT_ReadSector										; Read sector
	pop		ebx													; Pop offset into sector
	jc 		SHORT .error										; If error, return

	; Sector is now read to memory
	mov		bl, BYTE [FAT_Type]									; Read FAT type
	cmp		bl, $20												; Is FAT32?
	je		.FAT32												; If so, jump

	xor		eax, eax											; Clear EAX
	mov		ax, WORD [FAT_ReadBuffer+ebx]						; Read FAT16 entry
	jmp		.done												; Skip over read below

.FAT32:
	mov		eax, DWORD [FAT_ReadBuffer+ebx]						; Read FAT32 entry

.done:
	ret

.error:
	stc															; Set carry
	ret

;========================================================================================
; Reads a logical sector from the drive.
; eax: Logical sector
; Clears the carry flag if successful, set otherwise.
;========================================================================================
FAT_ReadSector:
	add		eax, DWORD [FAT_PartitionOffset]					; Add partition offset

	mov		ebx, DWORD [FAT_LastLoadedSector]					; Read last sector we read from HDD
	cmp		eax, ebx											; Are we getting a request to read same sector?
	je		.done												; If they are the same sector, branch

	mov		DWORD [FAT_ERTable+0x08], eax						; Write LBA
	mov		WORD [FAT_ERTable+0x02], 0x01						; Read one sector
	mov		WORD [FAT_ERTable+0x04], FAT_ReadBuffer				; Temporary sector buffer offset (seg 0)
	mov		WORD [FAT_ERTable+0x06], 0x00						; Page 0

	mov		DWORD [FAT_LastLoadedSector], eax					; Store LBA we're loading

	pusha														; Push registers (BIOS may clobber them)
	mov 	si, FAT_ERTable										; Address of "disk address packet"
	mov 	ah, $42												; Extended Read
	mov		dl, BYTE [FAT_Drive]								; Device number
	int 	$13													; Perform read
	popa														; Pop registers
	jc 		SHORT .error										; If error, return

.done:
	clc															; Clear carry
	ret

.error:
	stc															; Set carry
	ret

;========================================================================================
; Tries to locate a file with the name pointed to by in esi in the root directory of the
; FAT. If found, returns the first cluster of the file in eax, filesize in ebx, and
; clears carry. If not found, sets carry.
;
; Note that this only searches the short filename.
;========================================================================================
FAT_FindFileAtRoot:
	pusha														; Push regs
	mov		eax, DWORD [FAT_BPB_RootClus]						; Read root sector location
	call	FAT_ClusterToSector									; Convert cluster->sector
	call	FAT_ReadSector										; Read sector

	mov		edi, FAT_ReadBuffer									; FAT read buffer
	mov		cx, (512/32)										; Search the first 512/32 entries

.searchLoop:
	mov		al, BYTE [edi]										; Read first byte of string

	cmp		al, $0E5											; Is directory entry free?
	je		.fileEntryIgnore									; If so, branch
	cmp		al, $00												; Is directory entry free and last one?
	je		.notFound											; If so, exit loop.

	; Store pointers
	xor		eax, eax											; Clear EAX
	xor		edx, edx											; Clear EDX
	mov		eax, edi											; Copy read pointer to EAX
	mov		edx, esi											; Copy filename compare ptr to EDX

	; Compare filename
	push	cx													; Push loop counter
	mov		cx, $0B												; Filename is 11 bytes

.comparison:
	mov		bl, BYTE [eax]										; Read soruce ptr
	cmp		bl, BYTE [edx]										; Compare against target
	jne		.compareFailed										; If not equal, branch

	inc		eax													; Increment read pointer
	inc		edx													; Increment target pointer

	loop	.comparison											; Compare 11 bytes

	pop		cx													; Pop loop counter from stack
	jmp		SHORT .found										; File was found

.compareFailed:
	pop		cx													; Pop loop counter

.fileEntryIgnore:
	add		di, $20												; Read next entry
	loop	.searchLoop											; Loop through entries

; Drop down here once loop finishes: file not found.
.notFound:
	popa														; Pop registers
	stc															; Set carry
	ret

; We found the file
.found:
	mov		ax, WORD [edi+26]									; Read cluster low word
	mov		WORD [.clusterOfFile], ax							; ""
	mov		ax, WORD [edi+20]									; Read cluster high word
	mov		WORD [.clusterOfFile+2], ax							; ""

	mov		eax, DWORD [edi+28]									; Read filesize
	mov		DWORD [.sizeOfFile], eax							; ""

	popa														; Pop registers
	mov		eax, DWORD [.clusterOfFile]							; Read file's cluster
	mov		ebx, DWORD [.sizeOfFile]							; Read file's size
	clc															; Clear carry bit
	ret

	align	4
.clusterOfFile:
	dd		0
.sizeOfFile:
	dd		0

;========================================================================================
; Reads the file whose first cluster is in eax to es:di.
;
; Note that this function returns after reading a maximum of 256 chunks.
;========================================================================================
FAT_ReadFile:
	pusha														; Push all regisers

	call	FAT_ReadCluster										; Read cluster
	jc 		SHORT .error										; If error, return

;	xor		cx, cx												; Clear CX
;	mov		gs, cx												; Clear GS
;
;.readLoop:
;	call	FAT_ReadFAT											; Read FAT entry for current cluster
;	and		eax, $0FFFFFFF										; Ignore high nybble
;	cmp		eax, $0FFFFFF8										; End of chain marker?
;	jae		.done												; If so, branch (unsigned compare)
;
;	call	FAT_ReadCluster										; Read cluster
;	jc 		SHORT .error										; If error, return
;
;	mov		cx, gs												; Read GS
;	inc		cx													; Increment GS
;	mov		gs, cx												; Move back to GS
;
;	cmp		cl, $0FF											; Is it max? ($FF)
;	je		.error												; If so, we're done
;
;	jmp		.readLoop											; Loop until all sectors of the file are read

.done:
	popa														; Pop registers
	clc															; Clear carry
	ret

.error:
	popa														; Pop registers
	stc															; Set carry
	ret

;========================================================================================
; Reads a logical sector from the drive.
; eax: Logical cluster
; es:di: Memory location (Incremented after read)
; Clears the carry flag if successful, set otherwise.
;========================================================================================
FAT_ReadCluster:
	mov		DWORD [.origCluster], eax							; Store original cluster
	call	FAT_ClusterToSector									; Convert cluster->sector

	add		eax, DWORD [FAT_PartitionOffset]					; Add partition offset

	mov		ebx, DWORD [FAT_LastLoadedSector]					; Read last sector we read from HDD
	cmp		eax, ebx											; Are we getting a request to read same sector?
	je		.done												; If they are the same sector, branch
	mov		DWORD [FAT_LastLoadedSector], eax					; Save sector we're reading

	xor		bx, bx												; Clear BX
	mov		bl, BYTE [FAT_BPB_SectorsPerCluster]				; Read sectors/cluster
	mov		WORD [FAT_ERTable+0x02], $7F							; Write sectors/cluster 
	mov		DWORD [FAT_ERTable+0x08], eax						; Write LBA
	mov		WORD [FAT_ERTable+0x04], di							; Offset in segment
	mov		WORD [FAT_ERTable+0x06], es							; Segment

	pusha														; Push registers (BIOS may clobber them)
	mov 	si, FAT_ERTable										; Address of "disk address packet"
	mov 	ah, $42												; Extended Read		
	mov		dl, BYTE [FAT_Drive]								; Device number
	int 	$13													; Perform read
	popa														; Pop registers
	jc 		SHORT .error										; If error, return

	call	.incrementReadPtr									; Increment read pointer

.done:	
	mov		eax, DWORD [.origCluster]							; Restore original cluster
	clc															; Clear carry
	ret

.error:
	mov		eax, DWORD [.origCluster]							; Restore original cluster
	stc															; Set carry
	ret

.incrementReadPtr:
	xor		eax, eax											; Clear EAX
	mov		ax, WORD [FAT_BPB_BytesPerSec]						; Read sector length
	xor		ebx, ebx											; Clear EBX
	mov		bl, BYTE [FAT_BPB_SectorsPerCluster]				; Read bytes per sector

	mul		ebx													; EAX = value to add to write ptr
	add		di, ax												; Add cluster length to di
	jno		.noOverflow											; If overflow bit isn't set, branch

	mov		ax, es												; Read segment we're writing to
	add		ax, $1000											; Write in next segment
	mov		es, ax												; Write to segment register

.noOverflow:
	ret

	align	4
.origCluster:
	dd		0

;========================================================================================
; Data section
;========================================================================================
	align	4
FAT_BPB_BytesPerSec: ; file offset 11
	dw		0
FAT_BPB_FSInfo: ; file offset 48, FAT32 only
	dw		0
FAT_BPB_ExtFlags: ; file offset 40, FAT32 only
	dw		0
FAT_BPB_FSVer: ; file offset 42, FAT32 only
	dw		0

	align	4
FAT_BPB_RootClus: ; file offset 44, FAT32 only
	dd		0
FAT_BPB_SectorsPerCluster: ; file offset 13
	db		0

	align	2
FAT_BPB_ReservedSectors: ; file offset 14
	dw		0
FAT_BPB_NumFATs: ; file offset 16
	db		0

	align	2
FAT_BPB_RootEntCnt:	; file offset 17
	dw		0

	align	4
FAT_BPB_FATSz: ; file offset 22 if FAT16, 36 if FAT32 and word at 22 is 0
	dd		0

; Works with FAT32
; For the total sector count, try to read BPB_FATSz16 first (19), then the 32-bit at off 32
FAT_BPB_TotSec: ; file offset 32 if 19 = 0
	dd		0

; Calculated when identifying FS
FAT_RootDirSectors:
	dw		0

	align	4
FAT_DataSectors:
	dd		0
FAT_FirstDataSector:
	dd		0
FAT_FirstClusterLocation:
	dd		0
FAT_TotalClusters:
	dd		0

; Offset into drive in sectors to the start of the FAT partition
FAT_PartitionOffset:
	dd		0
FAT_Type:
	db		0

; BIOS drive to read from
FAT_Drive:
	db		0

	align	4
FAT_LastLoadedSector:
	dd		0

	align	4
FAT_ERTable:
	db	$10
	db	0
	dw	0														; Num blocks
	dw	0														; Dest
	dw	0														; Memory page
	dd	0														; Starting LBA
	dd	0	

	align	$10
FAT_ReadBuffer: