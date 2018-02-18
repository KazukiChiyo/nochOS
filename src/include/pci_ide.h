///     For now, we will only use the ATA PIO mode.
///     ATAPI interfaces might be implemented later.
///
/// Since the hard disk driver is not the focus of this project,
/// we believe that the code snippets from OSDev are good enough
/// implementation for this project. The two structures defined
/// in this file are exerpted from OSDev.
///
/// - reference: wiki.osdev.org/PCI_IDE_Controller
///

#ifndef _PCI_IDE_H
#define _PCI_IDE_H

#include <types.h>
#include <device.h>

#define BAR0 0x1F0
#define BAR1 0x3F6
#define BAR2 0x170
#define BAR3 0x376
#define BAR4 0xC040

extern device_t * ide_device;

///
/// Defines the I/O ports of an IDE channel.
///
typedef struct ide_channel {
    uint16_t base;
    uint16_t ctrl;
    uint16_t bm_ide;
    uint8_t n_ien;
} ide_channel_t;

///
/// Defines info of an IDE device.
///
typedef struct ide_device {
    uint8_t reserved;
    uint8_t channel;
    uint8_t drive;
    uint16_t type;
    uint16_t signature;
    uint16_t capabilities;
    uint32_t command_sets;
    uint32_t size;

    // Human readable string.
    uint8_t model[41];
} ide_device_t;

///
/// Reads data from port of an IDE channel.
///
uint8_t ide_read(uint8_t channel, uint8_t reg);

///
/// Writes data to a register of an IDE channel.
///
void ide_write(uint8_t channel, uint8_t reg, uint8_t data);

///
/// Reads data from IDE repeatedly into buffer.
///
void ide_read_buf(uint8_t channel, uint8_t reg, uint32_t buf,
                    uint32_t longs, uint16_t es);

///
/// Polls the status register.
///
uint8_t ide_poll(uint8_t channel, uint32_t advanced_check);

///
/// Prints error from an IDE device to screen.
///
uint8_t ide_err_print(uint32_t drive, uint8_t err);

///
/// Initializes IDE data structures.
///
void ide_init(void);

///
/// Reads from or writes to ATA drive.
///
uint8_t ide_ata_access(uint8_t rw, uint8_t drive, uint32_t lba_addr,
                        uint8_t nsects, uint16_t selector, uint32_t mem);

///
/// Blockingly waits until IRQ occurs.
///
void ide_wait_irq(void);

///
/// Reads from ATAPI drive.
///
uint8_t ide_atapi_read(uint8_t drive, uint32_t lba_addr, uint8_t nsects,
                        uint16_t es, uint32_t edi);

///
/// Writes to ATAPI drive.
///
uint8_t ide_atapi_write(uint8_t drive, uint32_t lba_addr, uint8_t nsects,
                        uint16_t es, uint32_t edi);

///
/// Reads sectors from IDE.
///
void ide_read_sectors(uint8_t drive, uint32_t lba_addr, uint8_t nsects,
                        uint16_t es, uint32_t edi);

///
/// Writes sectors to IDE.
///
void ide_write_sectors(uint8_t drive, uint32_t lba_addr, uint8_t nsects,
                        uint16_t es, uint32_t edi);

#endif /* _PCI_IDE_H */
