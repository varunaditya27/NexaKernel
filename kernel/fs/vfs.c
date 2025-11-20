/*
 * kernel/fs/vfs.c
 *
 * Virtual filesystem abstraction. This module exposes a minimal VFS API and
 * delegates to ramfs or future filesystems. Implement `vfs_mount`,
 * `vfs_open`, `vfs_read` etc. Keep the interface stable for userland.
 */

#include <stdint.h>

void vfs_init(void) {
    // Initialize VFS layer and register ramfs
}
