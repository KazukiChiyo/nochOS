/// ext2.h: Interface for EXT2 file system
///
/// author: Zhengcheng Huang
/// reference: www.nongnu.org/ext2-doc/ext2.html
///

#ifndef _EXT2_H
#define _EXT2_H

#include <types.h>
#include <vfs.h>

///
/// Represents how a super block is stored on disk.
///
typedef struct ext2_super_block {
    uint32_t s_inodes;
    uint32_t s_blocks;
    uint32_t s_r_blocks;
    uint32_t s_free_blocks;
    uint32_t s_free_inodes;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    /* --- EXT2_DYNAMIC_REV Specific --- */
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incmpat;
    uint32_t s_freature_ro_compat;
    uint32_t s_uuid [4];
    uint32_t s_volume_name [4];
    uint32_t s_last_mounted [16];
    uint32_t s_algo_bitmap;

    /* --- Performance Hints --- */

    /* --- Journaling Support --- */

    /* --- Directory Indexing Support --- */

    /* --- Other Options --- */

/// Defined s_state values
#define EXT2_VALID_FS           1
#define EXT2_ERROR_FS           2

/// Defined s_errors values
#define EXT2_ERRORS_CONTINUE    1
#define EXT2_ERRORS_RO          1
#define EXT2_ERRORS_PANIC       1

/// Defined s_creator_os values
#define EXT2_0S_LINUX           0
#define EXT2_0S_HURD            1
#define EXT2_0S_MASIX           2
#define EXT2_0S_FREEBSD         3
#define EXT2_0S_LITES           4

/// Defined s_rev_level values
#define EXT2_GOOD_OLD_REV       0
#define EXT2_DYNAMIC_REV        1

} __attribute__((packed)) ext2_super_t;

///
/// Represents how a group descriptor table is stored on disk.
///
typedef struct block_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks;
    uint16_t bg_free_inodes;
    uint16_t bg_used_dirs;
    uint16_t bg_pad;
    uint8_t bg_reserved [12];
} __attribute__((packed)) ext2_bg_desc_t;

///
/// Represents how an inode is stored on disk.
///
typedef struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
/* Sacrificed for hacky encryption/decryption
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
*/
    uint8_t  aes_key [16];
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block [15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint32_t i_osd2 [3];

/// Defined i_mode values
#define EXT2_S_IFSOCK           0xC000
#define EXT2_S_IFLNK            0xA000
#define EXT2_S_IFREG            0x8000
#define EXT2_S_IFBLK            0x6000
#define EXT2_S_IFDIR            0x4000
#define EXT2_S_IFCHR            0x2000
#define EXT2_S_IFIFO            0x1000

/// Defined i_flags values
#define EXT2_SECRM_FL           0x1
#define EXT2_UNRM_FL            0x2
#define EXT2_COMPR_FL           0x4
#define EXT2_SYNC_FL            0x8
#define EXT2_IMMUTABLE_FL       0x10
#define EXT2_APPEND_FL          0x20
#define EXT2_NODUMP_FL          0x40
#define EXT2_NOATIME_FL         0x80
#define EXT2_DIRTY_FL           0x100
#define EXT2_COMPRBLK_FL        0x200
#define EXT2_NOCOMPR_FL         0x400
#define EXT2_ECOMPR_FL          0x800
#define EXT2_BTREE_FL           0x1000
#define EXT2_INDEX_FL           0x1000
#define EXT2_IMAGIC_FL          0x2000
#define EXT2_JOURNAL_DATA_FL    0x4000
#define EXT2_RESERVED_FL        0x80000000

/// Defined reserved Inodes
#define EXT2_BAD_INO            1
#define EXT2_ROOT_INO           2
#define EXT2_ACL_IDX_INO        3
#define EXT2_ACL_DATA_INO       4
#define EXT2_BOOT_LOADER_INO    5
#define EXT2_UNDEL_DIR_INO      6

} __attribute__((packed)) ext2_inode_t;

typedef struct ext2_dentry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    int8_t name [255];

/// Defined file types
#define EXT2_FT_UNKNOWN         0
#define EXT2_FT_REG_FILE        1
#define EXT2_FT_DIR             2
#define EXT2_FT_CHRDEV          3
#define EXT2_FT_BLKDEV          4
#define EXT2_FT_FIFO            5
#define EXT2_FT_SOCK            6
#define EXT2_FT_SYMLINK         7

} __attribute__((packed)) ext2_dentry_t;

static inline int32_t imode_to_ft(uint16_t i_mode) {
    switch (i_mode >> 12) {
        case 0xC: return EXT2_FT_SOCK;
        case 0xA: return EXT2_FT_SYMLINK;
        case 0x8: return EXT2_FT_REG_FILE;
        case 0x6: return EXT2_FT_BLKDEV;
        case 0x4: return EXT2_FT_DIR;
        case 0x2: return EXT2_FT_CHRDEV;
        case 0x1: return EXT2_FT_FIFO;
        default: return EXT2_FT_UNKNOWN;
    }
}

extern void ext2_init(void);

extern file_op_t * ext2_file_fop;
extern file_op_t * ext2_dir_fop;
extern inode_op_t * ext2_iop;

#endif /* _EXT2_H */

