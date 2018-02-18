/// ext2.c: EXT2 implementation.
///
/// - author: Zhengcheng Huang
/// - reference: www.nongnu.org/ext2-doc/ext2.html
///

#include <ext2.h>
#include <vfs.h>
#include <pci_ide.h>
#include <lib.h>
#include <bitmap.h>
#include <mem.h>

#define EXT2_SUPER_LBA  0x3F

#define ext2_sb \
    ((ext2_super_t *)(superblock.priv_data))

#define ext2_read_bitmap(blk_buf, bitmap_blkno, bitmap_name) \
do {\
    ext2_bg_desc_t * bg_desc;\
\
    if (0 != ext2_read_block(2, blk_buf)) {\
        return -1;\
    }\
    bg_desc = (ext2_bg_desc_t *)blk_buf;\
\
    bitmap_blkno = bg_desc->bitmap_name;\
    if (0 != ext2_read_block(bitmap_blkno, blk_buf)) {\
        return -1;\
    }\
} while (0)


/// --- EXT2 file methods declaration --- ///

/// File operations for regular files
static int32_t file_fopen(file_t * self, const int8_t * fname);
static int32_t file_fread(file_t * self, void * buf, uint32_t nbytes);
static int32_t file_fwrite(file_t * self, const void * buf, uint32_t nbytes);
static int32_t file_fclose(file_t * self);
static int32_t file_fseek(file_t * self, int32_t offset, int32_t whence);
static int32_t file_getkey(file_t * self, uint8_t * key);
static int32_t file_setkey(file_t * self, uint8_t * key);

/// File operations for directories
static int32_t dir_fread(file_t * self, void * buf, uint32_t nbytes);
static int32_t dir_fwrite(file_t * self, const void * buf, uint32_t nbytes);

/// --- EXT2 inode methods declaration --- ///

/// inode operations for mod inodes.
static int32_t ilookup(inode_t * self, dentry_t * dentry, const int8_t * fname);
static int32_t icreate(inode_t * self, dentry_t * dentry, uint16_t mode);
static int32_t iremove(inode_t * self, const int8_t * fname);
static int32_t imkdir(inode_t * self, const int8_t * dirname);

/// --- EXT2 file operation jumptables --- ///

static file_op_t ext2_file_fops = {
    .open = file_fopen,
    .read = file_fread,
    .write = file_fwrite,
    .close = file_fclose,
    .seek = file_fseek,
    .getkey = file_getkey,
    .setkey = file_setkey
};

static file_op_t ext2_dir_fops = {
    .open = file_fopen,
    .read = dir_fread,
    .write = dir_fwrite,
    .close = file_fclose,
    .seek = file_fseek,
    .getkey = file_getkey,
    .setkey = file_setkey
};

/// --- EXT2 inode operation jumptable --- ///

static inode_op_t ext2_iops = {
    .lookup = ilookup,
    .create = icreate,
    .remove = iremove,
    .mkdir = imkdir
};

/// --- EXT2 static variables --- ///

static ext2_inode_t cur_ext2_dir;
static ext2_super_t ext2_super;

/// --- EXT2 interface --- ///

file_op_t * ext2_file_fop = &ext2_file_fops;
file_op_t * ext2_dir_fop = &ext2_dir_fops;
inode_op_t * ext2_iop = &ext2_iops;

/// --- EXT2 helpers declaration --- ///

///
/// Finds the inode number of the first free inode.
///
/// - author: Zhengcheng Huang
///
/// - return:
///     Inode number of the next free inode location.
///
/// - side effect:
///     Mark the inode number as used.
///
static int32_t next_free_ino(void);

///
/// Finds the block number of the first free data block.
///
/// - author: Zhengcheng Huang
///
/// - return:
///     Block number of the next free data block.
///
/// - side effect:
///     Mark the block number as used.
///
static int32_t next_free_blkno(void);

///
/// Reads data from EXT2 file into buffer.
/// File is read from byte indexed by given offset,
/// A given number of bytes will be read.
///
/// - author: Zhengcheng
///
/// - arguments:
///     inode: EXT2 inode representing the file.
///     offset: Index of the first byte to be read.
///     buf: The buffer to be filled.
///     nbytes: Number of bytes to be read.
///
/// - return:
///     -1 ~ on failure
///     0  ~ nothing is read
///     n  ~ number of bytes read from file
///
static int32_t ext2_read_data(const ext2_inode_t *, uint32_t, void *, uint32_t);

///
/// Reads data from a EXT2 directory entry into a VFS dentry object.
/// The directory entry is specified by its name.
/// The directory containing the entry is represented by given EXT2 inode struct.
///
/// - author: Zhengcheng Huang
///
/// - arguments
///     fname: File name of the directory entry.
///     inode: EXT2 inode representing the directory containing the entry.
///     dentry: VFS dentry object to be filled.
///
/// - return:
///     -1 ~ failure
///     0  ~ if the specified file is not found
///     n  ~ the filename length
///
/// - side effects: None.
///
static int32_t ext2_read_dentry_by_name(const int8_t *, const ext2_inode_t *, dentry_t *);

///
/// Reads data from a EXT2 directory entry into a VFS dentry struct.
/// The directory entry is specified by its index inside its directory.
/// The directory is represented by given EXT2 inode struct.
///
/// - author: Zhengcheng Huang
///
/// - arguments
///     idx: Zero-based index of the directory entry inside directory.
///     inode:
///         EXT2 inode representing the directory containing the
///         directory entry.
///     dentry:
///         VFS dentry struct to be filled.
///
/// - return:
///     -1 ~ failure
///     0  ~ if no file has the specified index
///     n  ~ the filename length
///
static int32_t ext2_read_dentry_by_index(uint32_t, const ext2_inode_t *, dentry_t *);

///
/// Reads data from an inode into an EXT2 inode struct.
///
/// - author: Zhengcheng Huang
///
/// - arguments
///     ino: Inode number of the inode in the file system.
///     inode: The EXT2 inode struct to be filled.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
static int32_t ext2_read_inode(uint32_t, ext2_inode_t *);

///
/// Writes EXT2 inode to block.
///
/// - author: Zhengcheng Huang
/// - arguments
///     ino: Inode number of the inode in the file system.
///     inode: The EXT2 inode struct to write.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
static int32_t ext2_write_inode(uint32_t, const ext2_inode_t *);

///
/// Reads data from file (represented by inode) into buffer with given
/// offset (first byte to read) and length (in bytes).
///
/// Assumes that all data are stored in direct data blocks.
///
/// - author: Zhengcheng Huang
///
/// - arguments:
///     inode:
///         The inode representing the file.
///     offset:
///         Offset in file starting from which to read data.
///         Must not exceed the limit of direct data blocks.
///     buf:
///         Buffer to copy data into.
///     nbytes:
///         Number of bytes to read from file.
///         Must not exceed the limit of direct data blocks.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
static int32_t ext2_read_direct(const ext2_inode_t *, uint32_t, void *, uint32_t);

///
/// Reads data from file (represented by inode) into buffer with given
/// offset (first byte to read) and length (in bytes).
///
/// Assumes that all data are stored in singly indirect data blocks.
///
/// - author: Zhengcheng Huang
///
/// - arguments:
///     inode:
///         The inode representing the file.
///     offset:
///         Offset in file starting from which to read data.
///         Must be within limits of singly direct data blocks.
///     buf:
///         Buffer to copy data into.
///     nbytes:
///         Number of bytes to read from file.
///         Must be within limits of singly direct data blocks.
///
/// - return:
///     -1 ~ failure
///     0  ~ nothing is read
///     n  ~ number of bytes read
///
int32_t ext2_read_indirect(const ext2_inode_t *, uint32_t, void *, uint32_t);

///
/// Writes data from buffer into file at given offset and length.
/// The following relations must hold:
///     offset + nbytes = inode->i_size,
///     inode->i_size < blocksize * 13
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
/// - side effects:
///     Writes to a direct data blocks of an inode that need to be written.
///
static int32_t ext2_write_direct(const ext2_inode_t *, uint32_t, const void *, uint32_t);

///
/// Read an EXT2 block into buffer.
///
/// - author: Zhengcheng Huang
/// - arguments
///     blkno: Block number in the entire file system.
///     buf: Buffer to copy to.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
static int32_t ext2_read_block(uint32_t, void *);

///
/// Read an EXT2 block into buffer with given offset and length.
///
/// - author: Zhengcheng Huang
/// - arguments
///     blkno: Block number in the entire file system.
///     buf: Buffer to copy to.
///     offset: First byte to copy.
///     nbytes: Number of bytes to copy.
///             Must be smaller than EXT2 block size.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
static int32_t ext2_read_block_bytes(uint32_t, void *, uint32_t, uint32_t);

///
/// Write buffer to EXT2 block.
///
/// = author: Zhengcheng Huang
/// - arguments
///     blkno: Block number in the entire file system.
///     buf: Buffer to write.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
/// - side effects:
///     Writes one block.
///
static int32_t ext2_write_block(uint32_t, const void *);

///
/// Write buffer to EXT2 block with given offset and length.
///
/// - author: Zhengcheng Huang
/// - arguments
///     blkno: Block number in the entire file system.
///     buf: Buffer to write.
///     offset: First byte to be written.
///     nbytes: Number of bytes to write.
///             Must be smaller than EXT2 block size.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
/// - side effects:
///     Write one block.
///
static int32_t ext2_write_block_bytes(uint32_t, const void *, uint32_t, uint32_t);

///
/// Allocates an inode with given EXT2 inode struct, file size,
/// and inode number. If the given file size requires more data blocks than
/// the inode currently has, allocate new data blocks.
///
/// - author: Zhengcheng Huang
/// - arguments
///     ino: The inode number.
///     inode:
///         This inode struct contains all information of the allocated
///         inode EXCEPT file size, data block count, and data block numbers.
///     fsize:
///         The file size of the allocated inode.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
/// - side effects:
///     Writes the block containing the inode.
///     Mark the inode number as used.
///     Mark the data blocks that will be used by the inode as used.
///
static int32_t ext2_alloc_inode(uint32_t ino, ext2_inode_t * inode, uint32_t fsize);

///
/// Allocates one data block for an inode and update the representing
/// EXT2 inode struct.
///
/// Note that the inode number is not given because data blocks are
/// allocated independently of inode numbers.
///
/// - arguments
///     inode: The EXT2 inode struct to update.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
/// Side effects:
///     Marks the allocated data block as used by updating the bitmap.
///
static int32_t ext2_alloc_iblock(ext2_inode_t * inode);

///
/// Tests if an inode number is used and tries to claim it.
///
/// - author: Zhengcheng Huang
/// - arguments
///     ino: The inode number to test and try.
///
/// - return:
///     0 ~ The inode number is already used.
///     1 ~ The inode number is free and then claimed by this function.
///
/// - side effects:
///     Marks the inode number as used by updating the bitmap.
///
static int32_t ino_try_set(uint32_t ino);

///
/// Tests if an inode number is used.
///
/// - author: Zhengcheng Huang
/// - return:
///     0 ~ no
///     1 ~ yes
///
static int32_t ino_exist(uint32_t ino);

///
/// Inserts a given new dentry into a given directory.
///
/// - author: Zhengcheng Huang
/// - arguments
///     dentry: The new dentry to insert.
///     inode: The EXT2 inode of the directory into which the new dentry will
///            be inserted.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
/// - side effects:
///     If the new dentry will use a new data block, mark the data block
///     as used.
///
/// - important:
///     This function will NOT allocate an inode for the inserted dentry.
///
static int32_t ext2_insert_dentry(const dentry_t * dentry, ext2_inode_t * inode);

///
/// Removes the dentry with given name from the given directory.
///
/// - author: Zhengcheng Huang
/// - arguments
///     fname: The file name of the removed dentry.
///     inode: The EXT2 inode of the directory from with to remove dentry.
///
/// - return:
///     -1 ~ failure
///     0  ~ success
///
/// - side effects:
///     Releases all data blocks related to the removed dentry.
///     If the removed dentry is the only dentry in a block, release
///     that block.
static int32_t ext2_remove_dentry(const int8_t * fname, ext2_inode_t * inode);

static int32_t release_ino(uint32_t ino);

static int32_t release_blkno(uint32_t blkno);

///
/// Tells whether `that` is ancestor of `this`.
///
static int32_t ext2_is_ancestor(const ext2_inode_t * that, const ext2_inode_t * this);

/// --- EXT2 inode methods implementation --- ///

/// TODO: Add support for full pathname.
static int32_t ilookup(inode_t * self, dentry_t * dentry, const int8_t * fname) {
    ext2_inode_t inode;

    ext2_read_inode(self->i_ino, &inode);
    if (0 >= ext2_read_dentry_by_name(fname, &inode, dentry))
        return -1;

    return 0;
}

static int32_t icreate(inode_t * self, dentry_t * dentry, uint16_t mode) {
    dentry_t temp_dent;
    ext2_inode_t inode;

    ext2_read_inode(self->i_ino, &inode);

    // Cannot create file with no name.
    if (dentry->filename[0] == '\0')
        return -1;

    // Cannot create file with existing filename.
    if (ext2_read_dentry_by_name(dentry->filename, &inode, &temp_dent) != 0)
        return -1;

    dentry->d_inode.i_op = ext2_iop;
    dentry->d_inode.i_mode = mode;
    dentry->d_inode.i_size = 0;
    dentry->d_inode.i_blocks = 0;
    dentry->d_inode.i_ino = next_free_ino();
    return ext2_insert_dentry(dentry, &inode);
}

static int32_t iremove(inode_t * self, const int8_t * fname) {
    ext2_inode_t inode;
    ext2_read_inode(self->i_ino, &inode);

    return ext2_remove_dentry(fname, &inode);
}

static int32_t imkdir(inode_t * self, const int8_t * dirname) {
    ext2_inode_t parent_inode, dir_inode;
    ext2_dentry_t this_dir, parent_dir;
    dentry_t dent;
    uint32_t parent_ino, dir_ino;

    ext2_read_inode(self->i_ino, &parent_inode);
    parent_ino = self->i_ino;

    strcpy(dent.filename, dirname);

    // Created a dir entry (which is a directory) in self.
    // It has an ino, and an allocated inode, which has nothing.
    self->i_op->create(self, &dent, 0x41FF);

    // Find the inode.
    dir_ino = dent.d_inode.i_ino;
    ext2_read_inode(dir_ino, &dir_inode);

    // Allocate a data block for the made directory.
    ext2_alloc_iblock(&dir_inode);

    // Initialize the new dir's inode.
    dir_inode.i_size = superblock.s_blocksize;
    dir_inode.i_mode = dent.d_inode.i_mode;
    ext2_write_inode(dir_ino, &dir_inode);

    // Write "." into new dir's data block.
    this_dir.inode = dir_ino;
    this_dir.rec_len = 12;
    this_dir.name_len = 1;
    this_dir.file_type = EXT2_FT_DIR;
    this_dir.name[0] = '.';
    ext2_write_block(dir_inode.i_block[0], &this_dir);

    // Write ".." into new dir's data block.
    parent_dir.inode = parent_ino;
    parent_dir.rec_len = 12;
    parent_dir.name_len = 2;
    parent_dir.file_type = EXT2_FT_DIR;
    parent_dir.name[0] = '.';
    parent_dir.name[1] = '.';
    ext2_write_block_bytes(dir_inode.i_block[0],&parent_dir,
                            this_dir.rec_len, parent_dir.rec_len);

    return 0;
}

/// --- EXT2 regular file file methods implementation --- ///

static int32_t file_fopen(file_t * self, const int8_t * fname) {
    dentry_t * base, * dent;
    int32_t res;
    base = (dentry_t *)malloc(sizeof(dentry_t));
    dent = (dentry_t *)malloc(sizeof(dentry_t));
    if (0 != (res = parse_path(fname, base, dent))) {
        if (res == 1)
            free_dentry(dent);
        return -1;
    }

    // Check file type and re-assign file operation jumptable accordingly.
    if (0 != (dent->d_inode.i_mode & EXT2_S_IFDIR))
        self->f_op = ext2_dir_fop;
    else if (0 != (dent->d_inode.i_mode & EXT2_S_IFREG))
        self->f_op = ext2_file_fop;

    // Update file object.
    self->f_dentry = *dent;
    self->f_pos = 0;
    free(dent);

    return 0;
}

static int32_t file_fread(file_t * self, void * buf, uint32_t nbytes) {
    uint32_t length;
    ext2_inode_t inode;

    // If file is not regular file, fail.
    if ((self->f_dentry.d_inode.i_mode & EXT2_S_IFREG) == 0)
        return -1;

    // Read inode, then read data from the inode.
    ext2_read_inode(self->f_dentry.d_inode.i_ino, &inode);
    length = ext2_read_data(&inode, self->f_pos, buf, nbytes);

    if (length == -1)
        return -1;

    // Update file position.
    self->f_pos += length;
    return length;
}

static int32_t file_fwrite(file_t * self, const void * buf, uint32_t nbytes) {
    uint32_t ino;
    ext2_inode_t inode;

    ino = self->f_dentry.d_inode.i_ino;
    ext2_read_inode(ino, &inode);

    ext2_alloc_inode(ino, &inode, self->f_pos + nbytes);

    ext2_write_direct(&inode, self->f_pos, buf, nbytes);

    self->f_pos += nbytes;

    return 0;
}

static int32_t file_fclose(file_t * self) {
    dentry_t * dent;
    dent = self->f_dentry.d_parent;
    free_dentry(dent);
    return 0;
}

static int32_t file_fseek(file_t * self, int32_t offset, int32_t whence) {
    int pos;
    switch (whence) {
        // Seek from beginning of file
        case SEEK_SET:
            pos = 0;
            break;

        // Seek from current file position
        case SEEK_CUR:
            break;

        // Seek from end of file
        case SEEK_END:
            pos = self->f_dentry.d_inode.i_size;
            break;

        default:
            return -1;
    }

    // Boundary check: upper limit.
    if (pos + offset > self->f_dentry.d_inode.i_size)
        return -1;

    // Boundary check: lower limit.
    if (pos + offset < 0)
        return -1;

    self->f_pos = pos + offset;
    return 0;
}

int32_t file_getkey(file_t * self, uint8_t * key) {
    ext2_inode_t inode;
    ext2_read_inode(self->f_dentry.d_inode.i_ino, &inode);
    memcpy(key, inode.aes_key, 16);
    return 0;
}

int32_t file_setkey(file_t * self, uint8_t *key) {
    ext2_inode_t inode;
    ext2_read_inode(self->f_dentry.d_inode.i_ino, &inode);
    memcpy(inode.aes_key, key, 16);
    ext2_write_inode(self->f_dentry.d_inode.i_ino, &inode);
    return 0;
}

/// --- EXT2 directory file methods implementation --- ///

static int32_t dir_fread(file_t * self, void * buf, uint32_t nbytes) {
    uint32_t length;
    ext2_inode_t inode;
    ext2_inode_t * iptr;
    dentry_t dent;

    // If file is not directory, fail.
    if ((self->f_dentry.d_inode.i_mode & EXT2_S_IFDIR) == 0)
        return -1;

    ext2_read_inode(self->f_dentry.d_inode.i_ino, &inode);
    iptr = &inode;

    // Read directory entry by index.
    length = ext2_read_dentry_by_index(self->f_pos, iptr, &dent);

    if (length == -1)
        return -1;

    // Calculate filename length and copy to buffer.
    if (nbytes < length)
        length = nbytes;
    strncpy((int8_t *)buf, dent.filename, length);

    // Update file object's file position.
    self->f_pos++;
    return length;
}

static int32_t dir_fwrite(file_t * self, const void * buf, uint32_t nbytes) {
    return -1;
}

/// --- EXT2 helpers implementation --- ///

#define ext2_access_block(blkno, buf, dev_method)\
do {\
    device_t * ide = superblock.s_dev;\
    uint32_t lba = EXT2_SUPER_LBA + blkno * 2;\
    return ide->d_op->dev_method(ide, buf, lba, superblock.s_blocksize);\
} while (0)

static int32_t ext2_access_block_bytes(uint32_t rw, uint32_t blkno, void * buf, uint32_t offset, uint32_t nbytes) {
    int32_t retval;

    // Temporary buffer to store data read directly from disk.
    uint8_t blk_buf [superblock.s_blocksize];

    // Hard disk device.
    device_t * ide = superblock.s_dev;

    // Calculate block's address on disk.
    uint32_t lba = EXT2_SUPER_LBA + blkno * 2;

    // Read from hard disk to temporary buffer, then copy specific bytes
    // (specified by offset and nbytes) to given buffer.
    retval = ide->d_op->read(ide, blk_buf, lba, superblock.s_blocksize);
    if (rw == 0) {
        memcpy(buf, blk_buf + offset, nbytes);
    } else {
        memcpy(blk_buf + offset, buf, nbytes);
        retval = ide->d_op->write(ide, blk_buf, lba, superblock.s_blocksize);
    }

    return retval;
}

static int32_t ext2_read_block(uint32_t blkno, void * buf) {
    // Read from hard disk using hard disk device.
    ext2_access_block(blkno, buf, read);
}

static int32_t ext2_read_block_bytes(uint32_t blkno, void * buf, uint32_t offset, uint32_t nbytes) {
    return ext2_access_block_bytes(0, blkno, buf, offset, nbytes);
}

static int32_t ext2_write_block(uint32_t blkno, const void * buf) {
    // Write to hard disk using hard disk device.
    ext2_access_block(blkno, buf, write);
}

static int32_t ext2_write_block_bytes(uint32_t blkno, const void * buf, uint32_t offset, uint32_t nbytes) {
    return ext2_access_block_bytes(1, blkno, (void *)buf, offset, nbytes);
}

static int32_t ext2_read_direct(const ext2_inode_t * inode, uint32_t offset, void * buf, uint32_t nbytes) {
    uint32_t iblkno; // 0-based block id relative to inode
    uint32_t buf_off; // offset inside a buf
    uint32_t cpy_len; // length to copy in each memcpy
    uint32_t rmn_size; // remaining file size
    uint32_t total_size; // total size read

    // Initialize variables.
    total_size = 0;
    rmn_size = inode->i_size - offset;
    iblkno = offset / superblock.s_blocksize;
    buf_off = offset % superblock.s_blocksize;

    // Read the block.

    while (1) {
        // Decide the length to copy.
        cpy_len = nbytes;
        if (superblock.s_blocksize - buf_off < cpy_len)
            cpy_len = superblock.s_blocksize - buf_off;
        if (rmn_size < cpy_len)
            cpy_len = rmn_size;

        // Copy part of file to buffer.
        ext2_read_block_bytes(inode->i_block[iblkno++], buf + total_size, buf_off, cpy_len);
        total_size += cpy_len;

        // If end of file is reached or all bytes requested are read.
        if (cpy_len == rmn_size || cpy_len == nbytes)
            return total_size;

        // Move on to the next block.
        buf_off = 0;
        rmn_size -= cpy_len;
        nbytes -= cpy_len;
    }
}

static int32_t ext2_write_direct(const ext2_inode_t * inode, uint32_t offset, const void * buf, uint32_t nbytes) {
    uint32_t iblkno;
    uint32_t buf_off;
    uint32_t cpy_len;
    uint32_t rmn_size;
    uint32_t total_size;

    // Initialize variables.
    total_size = 0;
    rmn_size = inode->i_size - offset;
    iblkno = offset / superblock.s_blocksize;
    buf_off = offset % superblock.s_blocksize;

    while (1) {
        // Decide number of bytes to copy
        cpy_len = nbytes;
        if (superblock.s_blocksize - buf_off < cpy_len)
            cpy_len = superblock.s_blocksize - buf_off;
        if (rmn_size < cpy_len)
            cpy_len = rmn_size;

        ext2_write_block_bytes(inode->i_block[iblkno++], buf + total_size, buf_off, cpy_len);
        total_size += cpy_len;

        if (cpy_len == rmn_size || cpy_len == nbytes)
            return total_size;

        buf_off = 0;
        rmn_size -= cpy_len;
        nbytes -= cpy_len;
    }
}

int32_t ext2_read_indirect(const ext2_inode_t * inode, uint32_t offset, void * buf, uint32_t nbytes) {
    // 0-based actual block index among singly indirect data blocks.
    uint32_t iblkno;
    // offset from start of singly indirect data blocks.
    uint32_t idr_off;
    uint32_t buf_off; // offset inside a block buffer
    uint32_t cpy_len; // length in bytes to copy
    uint32_t rmn_size; // size of the remaining file
    uint32_t total_size; // number of bytes read.
    // the indirect data block
    uint32_t idr_buf [superblock.s_blocksize / 4];

    // Initialize variables.
    total_size = 0;
    rmn_size = inode->i_size - superblock.s_blocksize * 12 - offset;
    idr_off = offset - superblock.s_blocksize * 12;
    iblkno = idr_off / superblock.s_blocksize;
    buf_off = idr_off % superblock.s_blocksize;
    ext2_read_block(inode->i_block[12], idr_buf);

    while (1) {
        // Decide number of bytes to copy
        cpy_len = nbytes;
        if (superblock.s_blocksize - buf_off < cpy_len)
            cpy_len = superblock.s_blocksize - buf_off;
        if (rmn_size < cpy_len)
            cpy_len = rmn_size;

        // Copy data to buffer.
        ext2_read_block_bytes(idr_buf[iblkno], buf, buf_off, cpy_len);
        total_size += cpy_len;

        // On reaching end of file or reading enough bytes, return.
        if (cpy_len == rmn_size || cpy_len == nbytes)
            return total_size;

        // Move on to the next block.
        buf_off = 0;
        rmn_size -= cpy_len;
        nbytes -= cpy_len;
        iblkno++;
    }
}

static int32_t ext2_access_inode(uint32_t rw, uint32_t ino, ext2_inode_t * inode) {
    uint8_t blk_buf [superblock.s_blocksize];
    uint32_t bgno, iidx, i_blkno, i_blkidx, bg_blkno;
    uint32_t offset, nbytes;
    ext2_bg_desc_t * bg_desc;

    if (1 != ino_exist(ino))
        return -1;

    // Locate the inode's block and offset inside the block.
    bgno = (ino - 1) / ext2_sb->s_inodes_per_group;
    iidx = (ino - 1) % ext2_sb->s_inodes_per_group;
    i_blkno = iidx / (superblock.s_blocksize / ext2_sb->s_inode_size);
    i_blkidx = iidx % (superblock.s_blocksize / ext2_sb->s_inode_size);

    // Locate the block group.
    bg_blkno = ext2_sb->s_blocks_per_group * bgno + 2;

    // Read block group from disk.
    if (0 != ext2_read_block(bg_blkno, blk_buf))
        return -1;
    bg_desc = (ext2_bg_desc_t *)blk_buf;

    // Read inode from disk.
    i_blkno += bg_desc->bg_inode_table;
    offset = i_blkidx * sizeof(ext2_inode_t);
    nbytes = sizeof(ext2_inode_t);

    // Read or write.
    if (rw == 0)
        return ext2_read_block_bytes(i_blkno, inode, offset, nbytes);
    else
        return ext2_write_block_bytes(i_blkno, inode, offset, nbytes);
}

static int32_t ext2_read_inode(uint32_t ino, ext2_inode_t * inode) {
    return ext2_access_inode(0, ino, inode);
}

static int32_t ext2_write_inode(uint32_t ino, const ext2_inode_t * inode) {
    return ext2_access_inode(1, ino, (ext2_inode_t *)inode);
}

static int32_t ext2_alloc_inode(uint32_t ino, ext2_inode_t * inode, uint32_t fsize) {
    uint32_t blocks;
    uint32_t blkno;
    uint32_t i;

    // Number of blocks needed to contain this file.
    // As a side effect, inode bitmap will be set for this ino.
    blocks = fsize / superblock.s_blocksize;
    if (fsize % superblock.s_blocksize)
        ++blocks;

    if (ino_try_set(ino) == 0)
        i = 0;
    else
        i = inode->i_blocks;

    // Fill i_block array with block numbers.
    // As a side effect, block bitmap will set for these blocks.
    inode->i_size = fsize;
    inode->i_blocks = blocks;
    for (; i < blocks; i++) {
        blkno = next_free_blkno();
        // TODO: -1?
        inode->i_block[i] = blkno;
    }

    // Write back to disk.
    // inode bitmap is already set by the time this line is reached.
    ext2_write_inode(ino, inode);
    return 0;
}

static int32_t ext2_alloc_iblock(ext2_inode_t * inode) {
    uint32_t blkno;

    if (inode->i_blocks >= 12)
        return -1;

    blkno = next_free_blkno();
    inode->i_block[inode->i_blocks] = blkno;
    inode->i_blocks++;
    return 0;
}

static void ext2_fill_vfs_dentry(dentry_t * vfs_dentry, const ext2_dentry_t * ext2_dentry) {
    ext2_inode_t the_inode;
    strncpy(vfs_dentry->filename, ext2_dentry->name, ext2_dentry->name_len);
    vfs_dentry->filename[ext2_dentry->name_len] = '\0';
    ext2_read_inode(ext2_dentry->inode, &the_inode);
    vfs_dentry->d_inode.i_op = ext2_iop;
    vfs_dentry->d_inode.i_blocks = the_inode.i_blocks;
    vfs_dentry->d_inode.i_size = the_inode.i_size;
    vfs_dentry->d_inode.i_ino = ext2_dentry->inode;
    vfs_dentry->d_inode.i_mode = the_inode.i_mode;
}

static int32_t ext2_read_dentry_by_name(const int8_t * fname, const ext2_inode_t * inode, dentry_t* dentry) {
    uint8_t blk_buf [superblock.s_blocksize];
    uint32_t dir_off; // Offset of dir entry inside blk_buf
    uint32_t blk_idx; // Which iblock
    uint32_t blkno;
    ext2_dentry_t * cur_dir;

    // Check if inode is directory.
    if (!(inode->i_mode & EXT2_S_IFDIR))
        return -1;

    dir_off = 0;
    blk_idx = 0;

    // Read data block.
    blkno = inode->i_block[blk_idx];
    ext2_read_block(blkno, blk_buf);
    while (blk_idx < 12 && blk_idx < inode->i_blocks) {
        // First boundary check
        if (dir_off >= 1024) {
            // If all blocks are exhausted, return 0 (nothing found)
            if (blk_idx + 1 >= inode->i_blocks)
                return 0;
            // Read new block and copy to temp dir.
            blkno = inode->i_block[++blk_idx];
            ext2_read_block(blkno, blk_buf);

            // Update cur_dir and dir_off.
            dir_off = 0;
        }
        cur_dir = (ext2_dentry_t *)(blk_buf + dir_off);

        if (EXT2_FT_UNKNOWN == cur_dir->file_type)
            return 0;

        // Move on to next dir.
        dir_off += cur_dir->rec_len;

        if (0 == strncmp(cur_dir->name, fname, cur_dir->name_len) &&
            strlen(fname) == cur_dir->name_len) {
            ext2_fill_vfs_dentry(dentry, cur_dir);
            return cur_dir->name_len;
        }
    }

    // Didn't find anything.
    return 0;
}

static int32_t ext2_read_dentry_by_index(uint32_t idx, const ext2_inode_t * inode, dentry_t * dentry) {
    uint8_t blk_buf [superblock.s_blocksize];
    uint32_t dir_off;
    uint32_t blk_idx;
    uint32_t blkno;
    int i;
    ext2_dentry_t * cur_dir;

    dir_off = 0;
    blk_idx = 0;

    // Read data block
    blkno = inode->i_block[blk_idx];
    ext2_read_block(blkno, blk_buf);

    // Step over all directory entries before the one at given index.
    for (i = 0; i <= idx; i++) {
        // On reaching end of block, move to next block.
        if (dir_off >= 1024) {
            // If all blocks are exhausted, return 0 (nothing read).
            if (blk_idx + 1 >= inode->i_blocks)
                return 0;
            blkno = inode->i_block[++blk_idx];
            ext2_read_block(blkno, blk_buf);
            dir_off = 0;
        }
        cur_dir = (ext2_dentry_t *)(blk_buf + dir_off);
        dir_off += cur_dir->rec_len;
    }

    // Fill the given VFS dentry.
    ext2_fill_vfs_dentry(dentry, cur_dir);
    return cur_dir->name_len;
}

static int32_t ext2_insert_dentry(const dentry_t * dentry, ext2_inode_t * inode) {
    uint8_t blk_buf [superblock.s_blocksize];
    uint32_t dir_off, blk_idx, blkno;
    uint32_t d_fname_len, d_raw_fname_len; // with/without padding
    uint32_t last_fname_len; // with padding
    uint32_t prev_rec_len, prev_true_rec_len;
    ext2_dentry_t * cur_dir;
    ext2_inode_t the_inode;

    dir_off = 0;
    blk_idx = 0;

    // Calculate filename length with/without padding for the inserted dentry.
    d_fname_len = strlen(dentry->filename);
    if (FNAME_LEN < d_fname_len)
        d_fname_len = FNAME_LEN;
    d_raw_fname_len = d_fname_len;

    // padding
    if (d_fname_len % 4 != 0)
        d_fname_len += 4 - d_fname_len % 4;

    blkno = inode->i_block[blk_idx];
    ext2_read_block(blkno, blk_buf);

    while (blk_idx < 12) {
        // First boundary check
        if (dir_off >= 1024) {
            // Update dir_off.
            dir_off = 0;

            // Check if last file is read.
            if (blk_idx + 1 >= inode->i_blocks) {
                ext2_alloc_iblock(inode);
                blkno = inode->i_block[++blk_idx];
                ext2_read_block(blkno, blk_buf);
                cur_dir = (ext2_dentry_t *)blk_buf;
                inode->i_size += superblock.s_blocksize;
                prev_true_rec_len = 0;
                cur_dir->rec_len = superblock.s_blocksize;
                break;
            }
            // Read new block and copy to temp dir.
            if (0 == inode->i_block[blk_idx + 1]) {
                inode->i_blocks = blk_idx + 1;
                ext2_alloc_iblock(inode);
            }
            blkno = inode->i_block[++blk_idx];
            ext2_read_block(blkno, blk_buf);
        }

        cur_dir = (ext2_dentry_t *)(blk_buf + dir_off);

        if (cur_dir->file_type == EXT2_FT_UNKNOWN) {
            prev_true_rec_len = 0;
            cur_dir->rec_len = superblock.s_blocksize - dir_off;
            break;
        }

        last_fname_len = cur_dir->name_len;
        if (last_fname_len % 4 != 0)
            last_fname_len += 4 - cur_dir->name_len % 4;

        // Found a vacuum to insert dentry.
        if (cur_dir->rec_len > last_fname_len + 8 + d_fname_len + 8) {
            prev_true_rec_len = last_fname_len + 8;
            break;
        }
        // Move on to next dir.
        dir_off += cur_dir->rec_len;
    }

    // Write dir entry to disk.
    prev_rec_len = cur_dir->rec_len;
    cur_dir->rec_len = prev_true_rec_len;
    cur_dir = (ext2_dentry_t *)(blk_buf + dir_off + prev_true_rec_len);
    cur_dir->inode = dentry->d_inode.i_ino;
    cur_dir->rec_len = prev_rec_len - prev_true_rec_len;
    cur_dir->name_len = d_raw_fname_len;
    cur_dir->file_type = imode_to_ft(dentry->d_inode.i_mode);
    strncpy(cur_dir->name, dentry->filename, d_raw_fname_len);
    ext2_write_block(blkno, blk_buf);

    // Create new inode on disk.
    the_inode.i_mode = dentry->d_inode.i_mode;
    the_inode.i_size = dentry->d_inode.i_size;
    the_inode.i_blocks = dentry->d_inode.i_blocks;
    ext2_alloc_inode(dentry->d_inode.i_ino, &the_inode, 0);
    return 0;
}

static int32_t ext2_remove_dentry(const int8_t * fname, ext2_inode_t * inode) {
    uint8_t blk_buf [superblock.s_blocksize];
    uint32_t dir_off; // Offset of dir entry inside blk_buf
    uint32_t blk_idx; // Which iblock
    uint32_t blkno;
    uint32_t i;
    int32_t result;
    ext2_dentry_t * cur_dir, * swp_dir;
    ext2_inode_t the_inode;
    ext2_inode_t rm_dir;

    // Check if inode is directory.
    if (!(inode->i_mode & EXT2_S_IFDIR))
        return -1;

    dir_off = 0;
    blk_idx = 0;

    // Read data block.
    blkno = inode->i_block[blk_idx];
    ext2_read_block(blkno, blk_buf);
    while (blk_idx < 12 && blk_idx < inode->i_blocks) {
        // First boundary check
        if (dir_off >= 1024) {
            // If all blocks are exhausted, return 0 (nothing found)
            if (blk_idx + 1 >= inode->i_blocks)
                return -1;
            // Read new block and copy to temp dir.
            blkno = inode->i_block[++blk_idx];
            ext2_read_block(blkno, blk_buf);

            // Update cur_dir and dir_off.
            dir_off = 0;
        }
        cur_dir = (ext2_dentry_t *)(blk_buf + dir_off);

        // No such file.
        if (EXT2_FT_UNKNOWN == cur_dir->file_type)
            return -1;

        // REMOVE the dentry.
        if (0 == strncmp(cur_dir->name, fname, cur_dir->name_len) &&
            strlen(fname) == cur_dir->name_len) {

            // If this is a directory?
            if (cur_dir->file_type == EXT2_FT_DIR) {
                // Read what's nested inside the dir.
                ext2_read_inode(cur_dir->inode, &rm_dir);
                result = ext2_read_dentry_by_index(2, &rm_dir, 0);
                if (result != 0)
                    return -1;
                if (ext2_is_ancestor(&rm_dir, inode))
                    return -1;
                release_blkno(rm_dir.i_block[0]);
            }

            // Free all blocks.
            ext2_read_inode(cur_dir->inode, &the_inode);
            for (i = 0; i < the_inode.i_blocks && i < 12; ++i) {
                release_blkno(the_inode.i_block[i]);
            }
            release_ino(cur_dir->inode);

            // Case 1: First dentry in a block, but not last one.
            if (dir_off == 0 && cur_dir->rec_len != superblock.s_blocksize) {
                // Copy the next dentry to this one, then annex the next dentry.
                swp_dir = (ext2_dentry_t *)(blk_buf + cur_dir->rec_len);
                cur_dir->inode = swp_dir->inode;
                cur_dir->file_type = swp_dir->file_type;
                cur_dir->name_len = swp_dir->name_len;
                cur_dir->rec_len = cur_dir->rec_len + swp_dir->rec_len;
                strncpy(cur_dir->name, swp_dir->name, swp_dir->name_len);
            }

            // Case 2: First and last dentry in a block.
            else if (dir_off == 0 && cur_dir->rec_len == superblock.s_blocksize) {
                release_blkno(blkno);
                inode->i_block[inode->i_blocks - 1] = 0;
                inode->i_blocks--;
            }

            // Case 3: Just a dentry
            else {
                // Be annexed by the previous dentry.
                swp_dir->rec_len = cur_dir->rec_len + swp_dir->rec_len;
            }

            ext2_write_block(blkno, blk_buf);
            return 0;
        }

        // Move on to next dir.
        swp_dir = cur_dir;
        dir_off += cur_dir->rec_len;
    }

    // No such file.
    return -1;
}

static int32_t ext2_read_data(const ext2_inode_t * inode, uint32_t offset, void * buf, uint32_t nbytes) {
//    if (offset >= 12 * superblock.s_blocksize) return 0;
/*
    if (offset + nbytes > 12 * superblock.s_blocksize)
        nbytes = 12 * superblock.s_blocksize - offset;
*/
//    return ext2_read_direct(inode, offset, buf, nbytes);
    if (offset >= inode->i_size) return 0;

    if (offset + nbytes > inode->i_size)
        nbytes = inode->i_size - offset;

    if (offset + nbytes <= 12 * superblock.s_blocksize)
        return ext2_read_direct(inode, offset, buf, nbytes);

    if (offset >= 12 * superblock.s_blocksize)
        return ext2_read_indirect(inode, offset, buf, nbytes);

    if (-1 == ext2_read_direct(inode, offset, buf, 12 * superblock.s_blocksize - offset))
        return -1;
    return ext2_read_indirect(inode, 12 * superblock.s_blocksize, 
                               buf + 12 * superblock.s_blocksize - offset, 
                           nbytes - (12 * superblock.s_blocksize - offset));

}

static int32_t next_free_ino(void) {
    uint8_t blk_buf[superblock.s_blocksize];
    uint32_t bitmap_blkno;
    int32_t bit;

    ext2_read_bitmap(blk_buf, bitmap_blkno, bg_inode_bitmap);

    // Find free inode from bitmap.
    bit = find_free_region((uint32_t *)blk_buf, superblock.s_blocksize * 4, 0);
    if (bit == -1)
        return -1;

    bitmap_set_bit((uint32_t *)blk_buf, bit);
    ext2_write_block(bitmap_blkno, blk_buf);
    return bit + 1;
}

static int32_t next_free_blkno(void) {
    uint8_t blk_buf[superblock.s_blocksize];
    uint32_t bitmap_blkno;
    int32_t bit;

    ext2_read_bitmap(blk_buf, bitmap_blkno, bg_block_bitmap);

    // Find free data block from bitmap.
    bit = find_free_region((uint32_t *)blk_buf, superblock.s_blocksize * 4, 0);
    if (bit == -1)
        return -1;

    bitmap_set_bit((uint32_t *)blk_buf, bit);
    ext2_write_block(bitmap_blkno, blk_buf);
    return bit + 1;
}

static int32_t ino_try_set(uint32_t ino) {
    uint8_t blk_buf [superblock.s_blocksize];
    uint32_t bitmap_blkno;

    ext2_read_bitmap(blk_buf, bitmap_blkno, bg_inode_bitmap);

    // Exist?
    if (bitmap_query_bit((uint32_t *)blk_buf, ino - 1) != 0)
        return 1;
    bitmap_set_bit((uint32_t *)blk_buf, ino - 1);
    ext2_write_block(bitmap_blkno, blk_buf);
    return 0;
}

static int32_t ino_exist(uint32_t ino) {
    uint8_t blk_buf [superblock.s_blocksize];
    uint32_t bitmap_blkno;

    if (ino == 0)
        return 0;

    ext2_read_bitmap(blk_buf, bitmap_blkno, bg_inode_bitmap);

    return bitmap_query_bit((uint32_t *)blk_buf, ino - 1);
}

static int32_t release_ino(uint32_t ino) {
    uint8_t blk_buf[superblock.s_blocksize];
    uint32_t bitmap_blkno;

    ext2_read_bitmap(blk_buf, bitmap_blkno, bg_inode_bitmap);

    bitmap_set_bit((uint32_t *)blk_buf, ino - 1);
    ext2_write_block(bitmap_blkno, blk_buf);
    return 0;
}

static int32_t release_blkno(uint32_t blkno) {
    uint8_t blk_buf[superblock.s_blocksize];
    uint32_t bitmap_blkno;

    ext2_read_bitmap(blk_buf, bitmap_blkno, bg_block_bitmap);

    bitmap_set_bit((uint32_t *)blk_buf, blkno - 1);
    ext2_write_block(bitmap_blkno, blk_buf);
    return 0;
}

static int32_t ext2_is_ancestor(const ext2_inode_t * that, const ext2_inode_t * this) {
    dentry_t d_that, d_this;
    ext2_inode_t * i_trav = (ext2_inode_t *)this;

    ext2_read_dentry_by_name(".", that, &d_that);
    ext2_read_dentry_by_name(".", this, &d_this);

    while (d_this.d_inode.i_ino != 2) {
        if (d_this.d_inode.i_ino == d_that.d_inode.i_ino)
            return 1;
        ext2_read_dentry_by_name("..", i_trav, &d_this);
        ext2_read_inode(d_this.d_inode.i_ino, i_trav);
    }
    return 0;
}

void ext2_init(void) {
    uint8_t buf [1024];

    // Set up block device.
    superblock.s_blocksize = 1024;
    superblock.s_dev = ide_device;

    // Bootstrap to EXT2 super block.
    ext2_read_block(1, buf);
    superblock.priv_data = &ext2_super;
    *ext2_sb = *((ext2_super_t *)buf);

    // Bootstrap to root inode.
    ext2_read_inode(EXT2_ROOT_INO, &cur_ext2_dir);
    superblock.s_root.d_inode.i_blocks = cur_ext2_dir.i_blocks;
    superblock.s_root.d_inode.i_size = cur_ext2_dir.i_size;
    superblock.s_root.d_inode.i_ino = EXT2_ROOT_INO;
    superblock.s_root.d_inode.i_mode = cur_ext2_dir.i_mode;
    superblock.s_root.d_inode.i_fop = &ext2_dir_fops;
    superblock.s_root.d_inode.i_op = &ext2_iops;
    superblock.s_root.d_parent = &superblock.s_root;

    // Set root filename.
    strcpy((int8_t *)superblock.s_root.filename, "/");

    // Set current directory.
    cur_dentry = superblock.s_root;
}
