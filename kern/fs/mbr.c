#include <types.h>
#include "mbr.h"
#include "io/disk.h"

ptable_t* mbr_load(disk_t *disk) {
	uint8_t *outBuff = (uint8_t *) kmalloc(0x200);
	ret = disk_read(disk, 0, 1, outBuff);

	if(ret != kDiskErrorNone) {
		return NULL;
	}

	ptable_t *table = (ptable_t *) kmalloc(sizeof(ptable_t));
	table->type = kPartitionTableMBR;
	table->disk = disk;

	return table;
}