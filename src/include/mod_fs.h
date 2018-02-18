/// fs.h: Interface for file system.
/// - author: Zhentcheng Huang

#ifndef _MOD_FS_H
#define _MOD_FS_H

#include <types.h>
#include <vfs.h>

#define MOD_IBLOCK_NUM 1023
#define MOD_FBLOCK_SIZE 4096
#define MOD_FNAME_LEN 32
#define MOD_DENTRY_NUM 63

///
/// Enumerates file types.
///
typedef enum mod_file_type_t {
    MFT_RTC, MFT_DIR, MFT_FILE
} mod_file_type_t;

///
/// Represents an inode object.
///
/// For MP3 file system, an inode contains a length in Bytes,
/// and an array of pointers to data blocks in the file system.
///
/// - important: By design, inode objects are used only for parsing
///              data in the file system, and should never be used
///              to copy and store data.
///
typedef struct mod_inode {
    int length;
    int iblock[MOD_IBLOCK_NUM];
} __attribute__((packed)) mod_inode_t;

///
/// Represents a dentry object.
///
/// For MP3 file system, an dentry object contains a filename, a
/// file type, and a pointer to an inode.
///
typedef struct mod_dentry {
    int8_t filename [MOD_FNAME_LEN]; // 32B filename
    int32_t type;
    uint32_t inode;

    uint32_t reserved [6];
} __attribute__((packed)) mod_dentry_t;

///
/// Represents a boot block object.
///
/// For MP3 file system, the boot block object contains the number
/// of dentries, number of inodes, number of data blocks, and an
/// array of dentries.
///
typedef struct mod_boot_block {
    uint32_t dir_entries;
    uint32_t inodes;
    uint32_t data_blocks;

    int32_t reserved [13]; // paddings

    mod_dentry_t dentry [MOD_DENTRY_NUM];
} __attribute__((packed)) mod_boot_block_t;

///
/// Jump table for file operations specific to individual files.
///
typedef struct mod_file_op_t {
    /// Called by the open system call to initialize file object.
	/// Assumes a file object is already allocated in the file descriptor
	/// array, but the file object itself is not initialized.
    int32_t (*open) (uint32_t fd, const int8_t * filename);

    /// Called by the read system call to perform a file read operation.
    int32_t (*read) (uint32_t fd, void * buf, uint32_t nbytes);

    /// Called by the write system call to perform a file write operation.
    int32_t (*write) (uint32_t fd, const void * buf, uint32_t nbytes);

    /// Called by the close system call to close file.
    /// Responsible for resetting the file object's flags and all shutdown
    /// operations related to the file object.
    int32_t (*close) (uint32_t fd);
} __attribute__((packed)) mod_file_op_t;

///
/// Structure representing an open file.
///
typedef struct mod_file_t {
    struct mod_file_op_t * f_op;
    uint32_t inode;
    uint32_t f_pos;
    int32_t flags;
} __attribute__((packed)) mod_file_t;

extern void mod_fs_root_init(dentry_t * dentry);

/// Reads a dentry object by its filename.
extern int32_t read_dentry_by_name(const int8_t * fname, mod_dentry_t * dentry);

/// Reads a dentry object by its index.
extern int32_t read_dentry_by_index(uint32_t index, mod_dentry_t * dentry);

// read size of the data block(s) by inode index
extern int32_t read_size_by_inode(uint32_t inode_idx);

/// Reads data from a file represented by given inode.
extern int32_t read_data(uint32_t inode, uint32_t offset, void * buf, uint32_t length);

/// Gives access to the file operation jumptables of regular files
/// and directories without exposing the implementations.
extern file_op_t * mod_file_fop;
extern file_op_t * mod_dir_fop;
extern inode_op_t * mod_iop;

#endif /* _FS_H */

