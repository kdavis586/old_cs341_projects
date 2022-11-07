/**
 * finding_filesystems
 * CS 341 - Fall 2022
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    inode * ino;
    if (valid_filename(path) && (ino = get_inode(fs, path))) {
        inode * ino = get_inode(fs, path);
        // zero out previous permissions
        u_int16_t type = ino->mode >> RWX_BITS_NUMBER << RWX_BITS_NUMBER;
        // combine new permissions with current type
        ino->mode = (type | new_permissions); // TODO: Does this actually work?
        clock_gettime(CLOCK_REALTIME, ino->ctim);
    } else {
        // path is not a valid format or no file associated with path
        errno = ENOENT;
        return -1;
    }
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    inode * ino;
    if (valid_filename(path) && (ino = get_inode(fs, path))) {
        inode * ino = get_inode(fs, path);
        if (owner != ((uid_t)-1)) {
            ino->uid = owner;
        }
        if (group != ((gid_t)-1)) {
            ino->gid = group;
        }
        clock_gettime(CLOCK_REALTIME, ino->ctim);
    } else {
        // path is not a valid format or no file associated with path
        errno = ENOENT;
        return -1;
    }
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    if (valid_filename(path) && !get_inode(fs, path)) {
        // path is valid and there is no inode associated with the current path
        inode_number unused_inode_num = first_unused_inode(fs);
        data_block_number unused_data_block_num = first_unused_data(fs);
        if (unused_inode_num == -1 || unused_data_block_num == -1) {
            // Didn't have resources to give out
            // TODO: finish this implementation
            return NULL;
        }


    }

    return NULL;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    return -1;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    return -1;
}
