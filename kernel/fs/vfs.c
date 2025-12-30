/*
 * ===========================================================================
 * kernel/fs/vfs.c
 * ===========================================================================
 *
 * Virtual File System (VFS) Layer
 *
 * This file implements the VFS abstraction layer, which provides a unified
 * interface for accessing different file systems. It handles mounting, file
 * descriptor management, and routing calls to the appropriate filesystem driver.
 *
 * Currently supports:
 * - RAMFS (in-memory filesystem) as the root filesystem
 *
 * Future support planned for:
 * - Block device filesystems (ext2, FAT)
 * - Network filesystems
 * - Device files (/dev)
 *
 * ===========================================================================
 */

#include "dsa_structures.h"
#include "../memory/memory.h"
#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * External RAMFS Functions
 * --------------------------------------------------------------------------- */

/* Inode types - must match ramfs.c definitions */
typedef enum {
    VFS_TYPE_FILE = 0,
    VFS_TYPE_DIRECTORY = 1
} vfs_node_type_t;

/* External RAMFS function declarations */
extern void ramfs_init(void);
extern int ramfs_create(const char *path, int type);
extern int ramfs_open(const char *path);
extern ssize_t ramfs_read(int fd, void *buffer, size_t size);
extern ssize_t ramfs_write(int fd, const void *buffer, size_t size);
extern int ramfs_close(int fd);
extern int ramfs_unlink(const char *path);
extern int ramfs_mkdir(const char *path);
extern int ramfs_touch(const char *path);
extern ssize_t ramfs_seek(int fd, ssize_t offset, int whence);
extern int ramfs_stat(const char *path, size_t *size, int *type);
extern int ramfs_list_dir(const char *path, char **names, size_t max_entries);
extern size_t ramfs_get_total_bytes(void);
extern bool ramfs_is_initialized(void);

/* ---------------------------------------------------------------------------
 * VFS State
 * --------------------------------------------------------------------------- */
static bool vfs_initialized = false;

/* ---------------------------------------------------------------------------
 * vfs_init - Initialize the Virtual File System
 * ---------------------------------------------------------------------------
 * Initializes all registered filesystems (currently only RAMFS).
 * --------------------------------------------------------------------------- */
void vfs_init(void)
{
    if (vfs_initialized) {
        return;
    }

    /* Initialize the root filesystem (RAMFS) */
    ramfs_init();

    vfs_initialized = true;
}

/* ---------------------------------------------------------------------------
 * VFS File Operations - Route to appropriate filesystem
 * ---------------------------------------------------------------------------
 * These functions provide a unified interface. Currently, all operations
 * are routed to RAMFS since it's the only mounted filesystem.
 * --------------------------------------------------------------------------- */

int vfs_open(const char *path)
{
    if (!vfs_initialized || path == NULL) {
        return -1;
    }
    return ramfs_open(path);
}

ssize_t vfs_read(int fd, void *buffer, size_t size)
{
    if (!vfs_initialized) {
        return -1;
    }
    return ramfs_read(fd, buffer, size);
}

ssize_t vfs_write(int fd, const void *buffer, size_t size)
{
    if (!vfs_initialized) {
        return -1;
    }
    return ramfs_write(fd, buffer, size);
}

int vfs_close(int fd)
{
    if (!vfs_initialized) {
        return -1;
    }
    return ramfs_close(fd);
}

int vfs_unlink(const char *path)
{
    if (!vfs_initialized || path == NULL) {
        return -1;
    }
    return ramfs_unlink(path);
}

int vfs_mkdir(const char *path)
{
    if (!vfs_initialized || path == NULL) {
        return -1;
    }
    return ramfs_mkdir(path);
}

int vfs_create(const char *path)
{
    if (!vfs_initialized || path == NULL) {
        return -1;
    }
    return ramfs_touch(path);
}

ssize_t vfs_seek(int fd, ssize_t offset, int whence)
{
    if (!vfs_initialized) {
        return -1;
    }
    return ramfs_seek(fd, offset, whence);
}

int vfs_stat(const char *path, size_t *size, int *type)
{
    if (!vfs_initialized || path == NULL) {
        return -1;
    }
    return ramfs_stat(path, size, type);
}

int vfs_list_dir(const char *path, char **names, size_t max_entries)
{
    if (!vfs_initialized || path == NULL || names == NULL) {
        return -1;
    }
    return ramfs_list_dir(path, names, max_entries);
}

/* ---------------------------------------------------------------------------
 * vfs_is_initialized - Check if VFS is ready
 * --------------------------------------------------------------------------- */
bool vfs_is_initialized(void)
{
    return vfs_initialized && ramfs_is_initialized();
}
