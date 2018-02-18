/// fs.c: Implementation of the mod file system.
/// - author: Zhengcheng Huang

#include <mod_fs.h>
#include <types.h>
#include <lib.h>
#include <vfs.h>
#include <mod_fs.h>
#include <rtc.h>

#define mod_iptr(inode) \
    ((mod_inode_t *)((uint32_t)boot_block + MOD_FBLOCK_SIZE + MOD_FBLOCK_SIZE * inode))

#define boot_block \
    ((mod_boot_block_t *)superblock.priv_data)

/// File operations for regular files
static int32_t file_fopen(file_t * self, const int8_t * fname);
static int32_t file_fread(file_t * self, void * buf, uint32_t nbytes);
static int32_t file_fwrite(file_t * self, const void * buf, uint32_t nbytes);
static int32_t file_fclose(file_t * self);

/// File operations for directories
static int32_t dir_fopen(file_t * self, const int8_t * fname);
static int32_t dir_fread(file_t * self, void * buf, uint32_t nbytes);
static int32_t dir_fwrite(file_t * self, const void * buf, uint32_t nbytes);
static int32_t dir_fclose(file_t * self);

/// inode operations for mod inodes.
static int32_t ilookup(inode_t * self, dentry_t * dentry, const int8_t * fname);

/// File operation jump table for regular files.
static file_op_t mod_file_fops = {
    .open = file_fopen,
    .read = file_fread,
    .write = file_fwrite,
    .close = file_fclose
};

/// File operation jump table for directories.
static file_op_t mod_dir_fops = {
    .open = dir_fopen,
    .read = dir_fread,
    .write = dir_fwrite,
    .close = dir_fclose
};

static inode_op_t mod_iops = {
    .lookup = ilookup
};

file_op_t * mod_file_fop = &mod_file_fops;
file_op_t * mod_dir_fop = &mod_dir_fops;
inode_op_t * mod_iop = &mod_iops;

static int32_t ilookup(inode_t * self, dentry_t * dentry, const int8_t * fname) {
    int32_t retval;
    mod_inode_t * miptr;
    mod_dentry_t mdent;

    retval  = read_dentry_by_name(fname, &mdent);
    if (retval == -1)
        return -1;

    miptr = mod_iptr(mdent.inode);

    strncpy(dentry->filename, mdent.filename, 32);
    dentry->d_inode.i_blocks = MOD_IBLOCK_NUM;
    dentry->d_inode.i_size = miptr->length;
    dentry->d_inode.i_ino = mdent.inode;
    dentry->d_inode.i_mode = mdent.type;
    dentry->d_inode.i_fop = &mod_file_fops;
    dentry->d_inode.i_op = &mod_iops;

    // Deliberately ignore d_op and private data.
    // We'll see their uses later.

    return 0;
}


///
/// VFS file open operation implementation for mod fs files.
/// We know for sure directories are not to be worried about.
///
static int32_t file_fopen(file_t * self, const int8_t * fname) {
    // The dentry to be read.
    dentry_t dent;
    // Directory to read from.
    inode_t * dir = &(superblock.s_root.d_inode);

    // Do the lookup, fill up dentry.
    if (0 != dir->i_op->lookup(dir, &dent, fname)) {
        return -1;
    }

    // Initialize file object.
    self->f_dentry = dent;
    self->f_pos = 0;

    switch (self->f_dentry.d_inode.i_mode) {
        case MFT_RTC:
            self->f_op = rtc_fop;
            break;
        case MFT_FILE:
            self->f_op = mod_file_fop;
            break;
        case MFT_DIR:
            self->f_op = mod_dir_fop;
            break;
        default:
            return -1;
    }

    // Deliberately ignore private data field.
    // We'll see its use later.

    return 0;
}

///
/// VFS file read operation implementation for mod fs files.
///
static int32_t file_fread(file_t * self, void * buf, uint32_t nbytes) {
    uint32_t bytes;
    uint32_t ino = self->f_dentry.d_inode.i_ino;

    bytes = read_data(ino, self->f_pos, buf, nbytes);

    if (bytes == -1)
        return -1;

    self->f_pos += bytes;

    return bytes;
}

///
/// VFS file write operation implementation for mod fs files.
/// Always fails because mod fs is read only.
///
static int32_t file_fwrite(file_t * self, const void * buf, uint32_t nbytes) {
    return -1;
}

///
/// Auto success.
///
static int32_t file_fclose(file_t * self) {
    return 0;
}

///
/// VFS file open operation implementation for mod fs directories.
/// We know for sure the directory is ".".
///
static int32_t dir_fopen(file_t * self, const int8_t * fname) {
    self->f_dentry = superblock.s_root;
    self->f_pos = 1;

    // Deliberately ignore private data field.
    // We'll see its use later.

    return 0;
}

///
/// VFS file read operation implementation for mod fs directories.
///
static int32_t dir_fread(file_t * self, void * buf, uint32_t nbytes) {
    mod_dentry_t mdent;

    if (self->f_pos >= boot_block->dir_entries)
        return 0;

    if (-1 == read_dentry_by_index(self->f_pos, &mdent)) {
        return -1;
    }

    if (nbytes > MOD_FNAME_LEN)
        nbytes = MOD_FNAME_LEN;

    self->f_pos++;
    strncpy(buf, mdent.filename, nbytes);
    return nbytes;
}

///
/// Auto fail.
///
static int32_t dir_fwrite(file_t * self, const void * buf, uint32_t nbytes) {
    return -1;
}

///
/// Auto success.
///
static int32_t dir_fclose(file_t * self) {
    return 0;
}

void mod_fs_root_init(dentry_t * dentry) {
    strncmp(dentry->filename, ".", MOD_FNAME_LEN);
    dentry->d_inode.i_blocks = 0;
    dentry->d_inode.i_size = 0;
    dentry->d_inode.i_ino = 0;
    dentry->d_inode.i_mode = MFT_DIR;
    dentry->d_inode.i_fop = &mod_dir_fops;
    dentry->d_inode.i_op = &mod_iops;
}

///
/// Fills given dentry object with file system dentry with given filename.
///
/// - author: Zhengcheng Huang
/// - arguments
///     fname: Filename of the dentry to read from file system.
///     dentry: The dentry object to be filled with the dentry read from
///             the file system.
/// - return: If given filename is not found, return -1.
///           Otherwise, return 0.
///
int32_t read_dentry_by_name(const int8_t * fname, mod_dentry_t * dentry) {
    int8_t * filename;
    uint32_t i;

    // Loop through all dentries to find if fname matches any dentry.
    for (i = 0; i < boot_block->dir_entries; i++) {
        filename = boot_block->dentry[i].filename;
        // If a match is found, fill dentry object and return success.
        if (strncmp(fname, filename, MOD_FNAME_LEN) == 0) {
            *dentry = boot_block->dentry[i];
            return 0;
        }
    }

    // No match found for fname, failure.
    return -1;
}

///
/// Fills given dentry object with file system dentry with given index.
///
/// - author: Zhengcheng Huang
/// - arguments
///     index: Index of the dentry to read from file system, starting 0.
///     dentry: The dentry object to be filled with the dentry read from
///             the file system.
/// - return: If given index is invalid, return -1.
///           Otherwise, return 0.
///
int32_t read_dentry_by_index(uint32_t index, mod_dentry_t * dentry) {
    // Check if index is valid. If not, return failure.
    if (index >= boot_block->dir_entries)
        return -1;

    // Fill dentry by index. Return success.
    *dentry = boot_block->dentry[index];
    return 0;
}


/* read_size_by_inode
   description: read size of file by inode index. NOTE: inode index is not a pointer to inode, it is the index number of the file.
   input: inode_idx - inode index
   output: none
   return value: size of the data block(s)
   side effect: none
   author: Kexuan Zou
*/
int32_t read_size_by_inode(uint32_t inode_idx) {
    if (inode_idx >= boot_block->inodes) return -1;

    /* find inode pointer; this is just to traverse inode_idx + 1 number of 4kb blocks */
    mod_inode_t* inode = (mod_inode_t *)((uint32_t)superblock.priv_data + MOD_FBLOCK_SIZE + MOD_FBLOCK_SIZE * inode_idx);
    return inode->length;
}

///
/// Fills given buffer with data read from the file represented by inode with
/// given inode number.
///
/// - author: Zhengcheng Huang
///
/// - arguments
///     inode:
///         The inode number of the inode of the file to be read from.
///     offset:
///         The offset in Bytes relative to the start of the file.
///     buf:
///         The buffer to be filled with data read from the file.
///     length:
///         The number of Bytes of data to be read.
///
/// - return:
///         If the given inode number or any data block number in the inode
///         is invalid, return -1.
///         Otherwise, return the number of bytes read before reaching the
///         end of file or before the specified number of Bytes of data is
///         read.
///
int32_t read_data(uint32_t inode, uint32_t offset, void * buf, uint32_t nbytes) {
    mod_inode_t * inode_mem = mod_iptr(inode);
    uint32_t iblock_idx = offset / MOD_FBLOCK_SIZE;
    uint32_t data_start = (uint32_t)boot_block + MOD_FBLOCK_SIZE * (1 + boot_block->inodes);
    uint32_t data_idx = inode_mem->iblock[iblock_idx];
    uint8_t * data_mem = (uint8_t *)(data_start + data_idx * MOD_FBLOCK_SIZE);
    uint32_t total_size = 0;
    uint32_t rmn_size = inode_mem->length - offset;
    uint32_t cpy_len = 0;
    uint32_t buf_off = offset % superblock.s_blocksize;

    while (1) {
        cpy_len = nbytes;
        if (superblock.s_blocksize - buf_off < cpy_len)
            cpy_len = superblock.s_blocksize - buf_off;
        if (rmn_size < cpy_len)
            cpy_len = rmn_size;

        memcpy(buf + total_size, data_mem + buf_off, cpy_len);
        total_size += cpy_len;

        if (cpy_len == rmn_size || cpy_len == nbytes)
            return total_size;

        buf_off = 0;
        rmn_size -= cpy_len;
        nbytes -= cpy_len;
        iblock_idx++;
        data_idx = inode_mem->iblock[iblock_idx];
        if (data_idx >= boot_block->data_blocks)
            return -1;
        data_mem = (uint8_t *)(data_start + data_idx * MOD_FBLOCK_SIZE);
    }
}
