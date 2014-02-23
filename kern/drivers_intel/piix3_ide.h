/*
 * Support for the PIIX3 chipset's IDE controller, supporting DMA operation and
 * other such fun.
 *
 * A system will only ever have a single PIIX3 IDE controller, so therefore
 * this driver does not support multiple devices and has static internal state
 * that should not be overridden.
 *
 * Note: Attempting to initialise this driver more than once will result in a
 * kernel panic.
 */

#ifndef PIIX3_IDE_H
#define PIIX3_IDE_H

#define PIIX3_IDE_VEN 0x8086
#define PIIX3_IDE_DEV 0x7010

typedef struct piix3_ide piix3_ide_t;

#endif