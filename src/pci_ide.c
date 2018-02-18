/// pci_ide.c: 
///     Implementation of the hard disk driver
///
/// Since the hard disk driver is not the focus of this project,
/// we believe that the code snippets from OSDev are good enough
/// implementation for this project.
///
/// In fact, most code in this file are exerpted from OSDev,
/// except the device abstraction (see device.h).
///
/// - reference: wiki.osdev.org/PCI_IDE_Controller
///

#include <pci_ide.h>
#include <ata.h>
#include <lib.h>
#include <x86_desc.h>
#include <time.h>
#include <device.h>

/// Size of one sector in Bytes
#define SECT_SIZE 512

/// Size of the identification space buffer in Bytes
#define IDENT_BUF_SIZE 128

static uint8_t ide_irq_invoked = 0;

static ide_channel_t ide_channels [2];
static ide_device_t ide_devices [4];
static uint8_t ide_buf [2048];

///
/// Device read function for IDE controller.
///
static int32_t ide_dev_read(device_t * self, void * buf, uint32_t offset, uint32_t nbytes) {
    int nsects;

    nsects = nbytes / SECT_SIZE;
    if (nbytes % SECT_SIZE != 0)
        nsects++;

    return ide_ata_access(ATA_READ, 0, offset, nsects, KERNEL_DS, (uint32_t)buf);
}

///
/// Device write function for IDE controller.
///
static int32_t ide_dev_write(device_t * self, const void * buf, uint32_t offset, uint32_t nbytes) {
    int nsects;
    nsects = nbytes / SECT_SIZE;
    if (nbytes % SECT_SIZE != 0)
        nsects++;

    return ide_ata_access(ATA_WRITE, 0, offset, nsects, KERNEL_DS, (uint32_t)buf);
}

static dev_op_t ide_dev_op = {
    .read = ide_dev_read,
    .write = ide_dev_write,
};

///
/// Device interface for IDE controller.
///
static device_t ide_dev = {
    .dev_name = "IDE Controller",
    .dev_id = 14,
    .d_op = &ide_dev_op,
    .priv_data = NULL
};

/// Pointer to the device interface, declared in header file.
device_t * ide_device = &ide_dev;

///
/// Reads data from IDE port (register).
///
/// - arguments
///     channel: The IDE channel to read from.
///     reg: The register to read from.
///
uint8_t ide_read(uint8_t channel, uint8_t reg) {
    uint8_t result;

    // Read back high order byte of the last
    // LBA48 value sent to an I/O port.
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 
                    ATA_CTRL_HOB | ide_channels[channel].n_ien);

    // Read from register.
    if (reg < 0x08)
        // If reading from base and using CHS, read.
        result = inb(ide_channels[channel].base + reg);
    else if (reg < 0x0C)
        // If reading from base and using LBA, subtract by 6.
        result = inb(ide_channels[channel].base + reg - 0x06);
    else if (reg < 0x0E)
        // If reading from ctrl, subtract by 10.
        result = inb(ide_channels[channel].ctrl + reg - 0x0A);
    else if (reg < 0x16)
        // If reading from bm_ide, subtract by 14.
        result = inb(ide_channels[channel].bm_ide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, ide_channels[channel].n_ien);

    return result;
}

///
/// Writes data from IDE port.
///
/// - arguments
///     channel: The IDE channel to read from.
///     reg: The register to write to.
///     data: The data to be written.
///
void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 
                    ATA_CTRL_HOB | ide_channels[channel].n_ien);

    if (reg < 0x08)
        outb(data, ide_channels[channel].base + reg);
    else if (reg < 0x0C)
        outb(data, ide_channels[channel].base + reg - 0x06);
    else if (reg < 0x0E)
        outb(data, ide_channels[channel].ctrl + reg - 0x0A);
    else if (reg < 0x16)
        outb(data, ide_channels[channel].bm_ide + reg - 0x0E);

    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, ide_channels[channel].n_ien);
}

///
/// Reads data from IDE port repeatedly into buffer.
///
/// - arguments
///     channel: The IDE channel to read from.
///     reg: The register to read from.
///     buffer: The buffer to copy read data into.
///     quads: The number of quads to read.
///
void ide_read_buf(uint8_t channel, uint8_t reg, uint32_t buf, 
        uint32_t longs, uint16_t es) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 
                ATA_CTRL_HOB | ide_channels[channel].n_ien);

    if (reg < 0x08)
        insl(ide_channels[channel].base + reg, es, buf, longs);
    else if (reg < 0x0C)
        insl(ide_channels[channel].base + reg - 0x06, es, buf, longs);
    else if (reg < 0x0E)
        insl(ide_channels[channel].ctrl + reg - 0x0A, es, buf, longs);
    else if (reg < 0x16)
        insl(ide_channels[channel].bm_ide + reg - 0x0E, es, buf, longs);

    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, ide_channels[channel].n_ien);
}

///
/// Polls the IDE controller by waiting the BUSY status to be cleared
///
/// - arguments
///     channel: The channel of the polled device.
///     advanced_check: Whether to check for device error.
///
uint8_t ide_poll(uint8_t channel, uint32_t advanced_check) {
    int i;

    // Delay 400 ns for BSY to be set.
    for (i = 0; i < 4; i++)
        // Reading Alternate Status port takes 100ns.
        ide_read(channel, ATA_REG_ALTSTATUS);

    // Wait for BSY to be cleared.
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ;

    // Check for device-related errors.
    if (advanced_check) {
        uint8_t state = ide_read(channel, ATA_REG_STATUS);

        // Error
        if (state & ATA_SR_ERR)
            return 1;

        // Device fault
        if (state & ATA_SR_DF)
            return 2;

        // DRQ
        if ((state & ATA_SR_DRQ) == 0)
            return 3;
    }

    // Success
    return 0;
}

///
/// Prints out IDE controller errors in readable format.
///
/// - arguments
///     drive: The device number (0-3)
///     err: Error code
///
uint8_t ide_err_print(uint32_t drive, uint8_t err) {
    int8_t * dev_channel;
    int8_t * dev_drive;
    int8_t * dev_channels [2] = {"Primary", "Secondary"};
    int8_t * dev_drives [2] = {"Master", "Slave"};

    if (err == 0)
        return 0;

    printf("IDE: ");
    if (err == 1) { printf("Device Fault\n"); err = 19; }
    else if (err == 2) {
        // Read error port
        uint8_t st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);

        // Pattern match error port read data.
        if (st & ATA_ER_AMNF)   { printf("No address mark fount\n"); err = 7; }
        if (st & ATA_ER_TK0NF)  { printf("No media or media error\n"); err = 3; }
        if (st & ATA_ER_ABRT)   { printf("Command aborted\n"); err = 20; }
        if (st & ATA_ER_MCR)    { printf("No media or media error\n"); err = 3; }
        if (st & ATA_ER_IDNF)   { printf("ID mark not fount\n"); err = 21; }
        if (st & ATA_ER_MC)     { printf("No media or media error\n"); err = 3; }
        if (st & ATA_ER_UNC)    { printf("Uncorrectable data error\n"); err = 22; }
        if (st & ATA_ER_BBK)    { printf("Bad sectors\n"); err = 13; }
    } 
    else if (err == 3)          { printf("Reads nothing\n"); err = 23; }
    else if (err == 4)          { printf("Write protected\n"); err = 8; }

    dev_channel = dev_channels[ide_devices[drive].channel];
    dev_drive = dev_drives[ide_devices[drive].drive];
    printf("- [%s %s] %s\n", dev_channel, dev_drive, ide_devices[drive].model);

    return err;
}

/// 
/// Initializes IDE controller.
///
/// The initialization process includes the following steps:
/// 1. For each ide_channel structure, initialize its IO port numbers.
/// 2. Detect devices on the channels and initialize ide_device
///    structures with the identification space information read
///    from the channels. 
///
void ide_init(void) {
    int i, j, k, count;
    uint8_t err, type, status;
    uint8_t cl, ch;

    // Set up I/O ports.
    ide_channels[ATA_PRIMARY].base = BAR0;
    ide_channels[ATA_PRIMARY].ctrl = BAR1;
    ide_channels[ATA_SECONDARY].base = BAR2;
    ide_channels[ATA_SECONDARY].base = BAR3;
    ide_channels[ATA_PRIMARY].bm_ide = BAR4;
    ide_channels[ATA_SECONDARY].bm_ide = BAR4 + 8;

    // Disable IRQs.
    ide_write(ATA_PRIMARY, ATA_REG_CONTROL, ATA_CTRL_NIEN);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, ATA_CTRL_NIEN);

    // Detect devices.
    count = 0;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            // Assuming no drive.
            ide_devices[count].reserved = 0;

            // Select drive.
            ide_write(i, ATA_REG_HDDEVSEL, ATA_HDDEVSEL_MASTER_CHS | (j << 4));
            sleep(1);

            // Send ATA identify command.
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            sleep(1);

            // Skip device if not existent.
            if (ide_read(i, ATA_REG_STATUS) == 0) continue;

            // Poll
            err = 0;
            while (1) {
                status = ide_read(i, ATA_REG_STATUS);
                if (status & ATA_SR_ERR)  { err = 1; break; }
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break;
            }

            type = IDE_ATA;

            // Probe for ATAPI devices
            if (err != 0) {
                cl = ide_read(i, ATA_REG_LBA_MID);
                ch = ide_read(i, ATA_REG_LBA_HIGH);
                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue;

                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                sleep(1);
            }

            // Read identification space of the device
            ide_read_buf(i, ATA_REG_DATA, (uint32_t)ide_buf, IDENT_BUF_SIZE, KERNEL_DS);

            // Read device parameters
            ide_devices[count].reserved     = 1;
            ide_devices[count].type         = type;
            ide_devices[count].channel      = i;
            ide_devices[count].drive        = j;
            ide_devices[count].signature
                = *((uint16_t *)(ide_buf + ATA_IDOFF_DEVICETYPE));
            ide_devices[count].capabilities
                = *((uint16_t *)(ide_buf + ATA_IDOFF_CAPABILITIES));
            ide_devices[count].command_sets
                = *((uint32_t *)(ide_buf + ATA_IDOFF_MAX_COMMANDSETS));

            // Get size
            if (ide_devices[count].command_sets & ATA_IDINFO_LBA48)
                // Supports 48-bit LBA addressing
                ide_devices[count].size
                    = *((uint32_t *)(ide_buf + ATA_IDOFF_MAX_LBA_EXT));
            else
                // Supports CHS or 28-bit LBA addressing
                ide_devices[count].size
                    = *((uint32_t *)(ide_buf + ATA_IDOFF_MAX_LBA));

            // Initialize human-readable model name.
            for (k = 0; k < 40; k += 2) {
                ide_devices[count].model[k] = ide_buf[ATA_IDOFF_MODEL + k + 1];
                ide_devices[count].model[k + 1] = ide_buf[ATA_IDOFF_MODEL + k];
            }
            ide_devices[count].model[40] = 0;

            count++;
        }
    }
/*
    for (i = 0; i < 4; i++) {
        if (ide_devices[i].reserved == 1) {
            printf("Drive %d:\n", i);
            printf(" Found %s drive %dMB - %s\n",
                    ((char *[]){"ATA", "ATAPI"})[ide_devices[i].type],
                    ide_devices[i].size / 1024 / 2,
                    ide_devices[i].model);
            printf(" Signature: %d\n", ide_devices[i].signature);
            printf(" Capabilities: %d\n", ide_devices[i].capabilities);
        }
    }
*/
}

///
/// Reads blocks of data from the hard disk to memory, or
/// writes blocks of data from memory to the hard disk.
///
/// - arguments:
///     rw: Read or write.
///     drive: The device number (0-3).
///     lba_addr: The LBA address of the accessed block on disk.
///     nsects: Number of sectors (blocks) to access.
///     selector: Segment selector of the memory location.
///     mem: The memory location to read to or write from.
///
uint8_t ide_ata_access(uint8_t rw, uint8_t drive, uint32_t lba_addr,
        uint8_t nsects, uint16_t selector, uint32_t mem) {
    uint8_t addr_mode, cmd;
    uint8_t addr_io [6];
    uint32_t channel = ide_devices[drive].channel;
    uint32_t slavebit = ide_devices[drive].drive;
    uint32_t bus = ide_channels[channel].base;
    uint32_t words = SECT_SIZE / 2;
    uint16_t cyl, i;
    uint8_t head, sect, err;

    ide_irq_invoked = 0;
    ide_channels[channel].n_ien = ATA_CTRL_NIEN;
    ide_write(channel, ATA_REG_CONTROL, ide_channels[channel].n_ien);

    // Select mode (LBA28, LBA48, CHS)
    if (lba_addr >= 0x10000000) {
        // LBA48
        addr_mode = 2;
        addr_io[0] = (lba_addr & 0xFF);
        addr_io[1] = (lba_addr & 0xFF00) >> 8;
        addr_io[2] = (lba_addr & 0xFF0000) >> 16;
        addr_io[3] = (lba_addr & 0xFF000000) >> 24;
        addr_io[4] = 0; // not needed
        addr_io[5] = 0; // not needed
        head = 0; // not used in LBA48
    } else if (ide_devices[drive].capabilities & 0x200) {
        // LBA28
        addr_mode = 1;
        addr_io[0] = (lba_addr & 0xFF);
        addr_io[1] = (lba_addr & 0xFF00) >> 8;
        addr_io[2] = (lba_addr & 0xFF0000) >> 16;
        addr_io[3] = 0;
        addr_io[4] = 0;
        addr_io[5] = 0;
        head = (lba_addr & 0xF000000) >> 24;
    } else {
        // CHS
        addr_mode = 0;
        sect = (lba_addr % 63) + 1;
        cyl = (lba_addr + 1 - sect) / (16 * 63);
        addr_io[0] = sect;
        addr_io[1] = cyl & 0xFF;
        addr_io[2] = (cyl >> 8) & 0xFF;
        addr_io[3] = 0;
        addr_io[4] = 0;
        addr_io[5] = 0;
        head = (lba_addr + 1 - sect) % (16 * 63) / 63;
    }

    // Poll the device
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ;

    // Select drive from controller
    if (addr_mode == 0)
        ide_write(channel, ATA_REG_HDDEVSEL, 
                slavebit ? ATA_HDDEVSEL_SLAVE_CHS : ATA_HDDEVSEL_MASTER_CHS | head);
    else
        ide_write(channel, ATA_REG_HDDEVSEL,
                slavebit ? ATA_HDDEVSEL_SLAVE_LBA : ATA_HDDEVSEL_MASTER_LBA | head);

    if (addr_mode == 2) {
        ide_write(channel, ATA_REG_SECCOUNT1, 0);
        ide_write(channel, ATA_REG_LBA_3, addr_io[3]);
        ide_write(channel, ATA_REG_LBA_4, addr_io[4]);
        ide_write(channel, ATA_REG_LBA_5, addr_io[5]);
    }
    ide_write(channel, ATA_REG_SECCOUNT0, nsects);
    ide_write(channel, ATA_REG_LBA_LOW, addr_io[0]);
    ide_write(channel, ATA_REG_LBA_MID, addr_io[1]);
    ide_write(channel, ATA_REG_LBA_HIGH, addr_io[2]);

    if (addr_mode <= 1 && rw == ATA_READ) cmd = ATA_CMD_READ_PIO;
    if (addr_mode == 2 && rw == ATA_READ) cmd = ATA_CMD_READ_PIO_EXT;
    if (addr_mode <= 1 && rw == ATA_WRITE) cmd = ATA_CMD_WRITE_PIO;
    if (addr_mode == 2 && rw == ATA_WRITE) cmd = ATA_CMD_WRITE_PIO_EXT;
    ide_write(channel, ATA_REG_COMMAND, cmd);

    // Read/write data
    if (rw == ATA_READ) {
        for (i = 0; i < nsects; i++) {
            err = ide_poll(channel, 1);
            if (err)
                return err;
            insw(bus, selector, mem, words);
            mem += (words * 2);
        }
    } else {
        for (i = 0; i < nsects; i++) {
            ide_poll(channel, 0);
            outsw(bus, selector, mem, words);
            mem += (words * 2);
        }
        if (addr_mode <= 1)
            ide_write(channel, ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        else
            ide_write(channel, ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH_EXT);

        ide_poll(channel, 0);
    }
    return 0;
}

