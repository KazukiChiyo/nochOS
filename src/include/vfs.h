/// vfs.h: Interface for the Virtual File System.
///
/// author: Zhengcheng Huang
///

#ifndef _VFS_H
#define _VFS_H

#include <device.h>

#define MAX_SUBDIRS 32
#define FNAME_LEN 48

#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

struct super_block;
struct file;
struct inode;
struct dentry;

/// Interface of an inode object.

struct inode_op;

typedef struct inode {
    /// Number of blocks of the file.
    uint32_t i_blocks;
    /// File size in Bytes.
    uint32_t i_size;
    /// inode number.
    uint32_t i_ino;
    /// File type
    uint32_t i_mode;

    /// Default file operation jump table.
    struct file_op * i_fop;
    /// inode operation jump table.
    struct inode_op * i_op;

    void * priv_data;
} inode_t;

typedef struct inode_op {
    int32_t (*lookup)(struct inode * self, struct dentry * dentry, const int8_t * fname);
    int32_t (*create)(struct inode * self, struct dentry * dentry, uint16_t mode);
    int32_t (*remove)(struct inode * self, const int8_t * fname);
    int32_t (*mkdir) (struct inode * self, const int8_t * dirname);
} inode_op_t;

/// Interface of a dentry object.

struct dentry_op;

typedef struct dentry {
    /// filename
    int8_t filename [FNAME_LEN];
    /// inode associated with the filename
    struct inode d_inode;

    /// dentry operation jump table.
    struct dentry_op * d_op;

    struct dentry * d_parent;

    void * priv_data;
} dentry_t;

/// Interface of a superblock object.

struct super_op;

typedef struct super_block {
    uint32_t s_blocksize;
    struct dentry s_root;
    struct super_op * s_op;
    struct device * s_dev;

    void * priv_data;
} super_block_t;

typedef struct super_op {
    int32_t (*read_inode)(struct inode * inode);
    int32_t (*write_inode)(struct inode * inode);
} super_op_t;

/// Interface of a file object.

struct file_op;

typedef struct file {
    struct dentry f_dentry;
    struct file_op * f_op;
    uint32_t f_pos;
    void * priv_data;
} file_t;

typedef struct file_op {
    int32_t (*open)(struct file * self, const int8_t * fname);
    int32_t (*read)(struct file * self, void * buf, uint32_t nbytes);
    int32_t (*write)(struct file * self, const void * buf, uint32_t nbytes);
    int32_t (*close)(struct file * self);
    int32_t (*seek)(struct file * self, int32_t offset, int32_t whence);
    int32_t (*getkey)(struct file * self, uint8_t * key);
    int32_t (*setkey)(struct file * self, uint8_t * key);
} file_op_t;

/// VFS functions

extern int32_t parse_path(const int8_t * path, struct dentry * parent, struct dentry * node);
extern void free_dentry(struct dentry * dentry);

/// Extern variables

extern struct super_block superblock;
extern struct dentry cur_dentry;

#endif /* _VFS_H */

