/// ata.h:
///     Definitions of the ATA PIO protocol.
///
/// Most macros defined in this file are taken from OSDev.
/// Since the hard disk driver is not the focus of our project,
/// we believe that the code snippets from OSDev are good enough
/// implementation for this project.
///
/// - reference: wiki.osdev.org/PCI_IDE_Controller
///

#ifndef _ATA_H
#define _ATA_H

/// IDE interface type
#define IDE_ATA     0x00
#define IDE_ATAPI   0x01

/// Master or slave
#define ATA_MASTER  0x00
#define ATA_SLAVE   0x01

/// IDE channels
#define ATA_PRIMARY     0x00
#define ATA_SECONDARY   0x01

/// IDE R/W direction
#define ATA_READ        0x00
#define ATA_WRITE       0x01

/// Bit masks to the status port
#define ATA_SR_BSY      0x80
#define ATA_SR_DRDY     0x40
#define ATA_SR_DF       0x20
#define ATA_SR_DSC      0x10
#define ATA_SR_DRQ      0x08
#define ATA_SR_CORR     0x04
#define ATA_SR_IDX      0x02
#define ATA_SR_ERR      0x01

/// Bit masks to the error port
#define ATA_ER_BBK      0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_IDNF     0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABRT     0x04
#define ATA_ER_TK0NF    0x02
#define ATA_ER_AMNF     0x01

/// Commands written to the command port
#define ATA_CMD_READ_PIO            0x20
#define ATA_CMD_READ_PIO_EXT        0x24
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_WRITE_PIO           0x30
#define ATA_CMD_WRITE_PIO_EXT       0x34
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_WRITE_DMA_EXT       0x35
#define ATA_CMD_CACHE_FLUSH         0xE7
#define ATA_CMD_CACHE_FLUSH_EXT     0xEA
#define ATA_CMD_PACKET              0xA0
#define ATA_CMD_IDENTIFY_PACKET     0xA1
#define ATA_CMD_IDENTIFY            0xEC

/// Identify command sent to select drive
#define ATA_HDDEVSEL_MASTER_LBA       0xE0
#define ATA_HDDEVSEL_SLAVE_LBA        0xF0
#define ATA_HDDEVSEL_MASTER_CHS       0xA0
#define ATA_HDDEVSEL_SLAVE_CHS        0xB0

/// Control bits written to the control port
#define ATA_CTRL_NIEN               0x02
#define ATA_CTRL_SRST               0x04
#define ATA_CTRL_HOB                0x80

/// ATAPI commands
#define ATAPI_CMD_READ              0xA8
#define ATAPI_CMD_EJECT             0x1B

/// Identification space information offset
#define ATA_IDOFF_DEVICETYPE        0
#define ATA_IDOFF_CYLINDERS         2
#define ATA_IDOFF_HEADS             6
#define ATA_IDOFF_SECTORS           12
#define ATA_IDOFF_SERIAL            20
#define ATA_IDOFF_MODEL             54
#define ATA_IDOFF_CAPABILITIES      98
#define ATA_IDOFF_FIELDVALID        106
#define ATA_IDOFF_MAX_LBA           120
#define ATA_IDOFF_MAX_COMMANDSETS   164
#define ATA_IDOFF_MAX_LBA_EXT       200

/// Information returned by Identify command
#define ATA_IDINFO_LBA48            0x04000000

/// IDE ports (offsets from BAR0/BAR2)
#define ATA_REG_DATA                0x00
#define ATA_REG_ERROR               0x01
#define ATA_REG_FEATURES            0x01
#define ATA_REG_SECCOUNT0           0x02
#define ATA_REG_LBA_LOW               0x03
#define ATA_REG_LBA_MID            0x04
#define ATA_REG_LBA_HIGH           0x05
#define ATA_REG_HDDEVSEL            0x06
#define ATA_REG_COMMAND             0x07
#define ATA_REG_STATUS              0x07
#define ATA_REG_SECCOUNT1           0x08
#define ATA_REG_LBA_3             0x09
#define ATA_REG_LBA_4             0x0A
#define ATA_REG_LBA_5            0x0B
#define ATA_REG_CONTROL             0x0C
#define ATA_REG_ALTSTATUS           0x0C
#define ATA_REG_DEVADDR             0x0D

#endif /* _ATA_H */
