/// vfs.c Implementation of the Virtual File System.
///
/// author: Zhengcheng Huang
///

#include <vfs.h>
#include <mem.h>
#include <lib.h>

super_block_t superblock;
dentry_t cur_dentry;

void free_dentry(dentry_t * dentry) {
    dentry_t * d_tmp;
    while (dentry != &superblock.s_root) {
        d_tmp = dentry;
        dentry = dentry->d_parent;
        free(d_tmp);
    }
}

int32_t parse_path(const int8_t * path, dentry_t * parent, dentry_t * node) {
    uint32_t i, j;
    int32_t retval;
    dentry_t * d_trav, * d_trav_cur, * d_tmp;
    inode_t * i_trav;

    int8_t name [FNAME_LEN];

    if (path[0] == '\0') return -1;

    if (path[0] == '/' || cur_dentry.filename[0] == '/') {
        d_trav = &superblock.s_root;
        if (path[0] == '/' && path[1] != '/')
            path += 1;
        if (path[0] == '/' && path[1] == '/')
            return -1;
    } else {
        // Copy current dentry.
        d_trav = (dentry_t *)malloc(sizeof(dentry_t));
        *d_trav = cur_dentry;
        // Pin current dentry.
        d_trav_cur = d_trav;

        // Copy each dentry along the path to root.
        while (d_trav->d_parent->filename[0] != '/') {
            d_tmp = d_trav->d_parent;
            d_trav->d_parent = (dentry_t *)malloc(sizeof(dentry_t));
            *(d_trav->d_parent) = *d_tmp;
            d_trav = d_trav->d_parent;
        }

        d_trav = d_trav_cur;
    }

    i_trav = &d_trav->d_inode;

    for (i = 0, j = 0; path[i] != '\0'; ++i) {
        // If file name is too long, fail.
        if (i >= FNAME_LEN) return -1;

        name[j++] = (path[i] == '/') ? '\0' : path[i];

        if (path[i] == '/' && path[i + 1] != '\0') {
            j = 0;
            // Invalid syntax.
            if (name[0] == '\0') {
                free_dentry(d_tmp);
                return -1;
            }
            // Self directory
            if (name[0] == '.' && name[1] == '\0') {
                i_trav = &d_trav->d_inode;
                continue;
            }
            // Parent directory
            if (name[0] == '.' && name[1] == '.' && name[2] == '\0') {
                d_tmp = d_trav;
                d_trav = d_trav->d_parent;
                free(d_tmp);
                i_trav = &d_trav->d_inode;
                continue;
            }
            // Next directory
            d_tmp = (dentry_t *)malloc(sizeof(dentry_t));
            d_tmp->d_parent = d_trav;
            if (-1 == i_trav->i_op->lookup(i_trav, d_tmp, name)) {
                free_dentry(d_tmp);
                return -1;
            }
            d_trav = d_tmp;
            i_trav = &d_trav->d_inode;
            j = 0;
        } else if (path[i] == '/') {
            if (name[0] == '\0') {
                free_dentry(d_tmp);
                return -1;
            }
            break;
        }
    }

    name[j] = '\0';

    // Memory cleanup.

    if (name[0] == '.' && name[1] == '\0') {
        *node = *d_trav;
        *parent = *(d_trav->d_parent);
        if (d_trav->d_parent != &superblock.s_root)
            free(d_trav->d_parent);
        retval = 0;
    }

    else if (name[0] == '.' && name[1] == '.' && name[2] == '\0') {
        *node = *(d_trav->d_parent);
        *parent = *(d_trav->d_parent->d_parent);
        if (d_trav->d_parent->d_parent != &superblock.s_root)
            free(d_trav->d_parent->d_parent);
        if (d_trav->d_parent != &superblock.s_root)
            free(d_trav->d_parent);
        retval = 0;
    }

    else if (name[0] == '\0') {
        *parent = superblock.s_root;
        *node = superblock.s_root;
        retval = 0;
    }

    else {
        *parent = *d_trav;
        if (-1 == i_trav->i_op->lookup(i_trav, node, name)) {
            strcpy(node->filename, name);
            retval = 1;
        } else
            retval = 0;
    }

    // Set parent of the end node
    node->d_parent = parent;

    if (d_trav != &superblock.s_root)
        free(d_trav);
    return retval;
}

