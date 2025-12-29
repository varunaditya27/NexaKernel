/*
 * ===========================================================================
 * kernel/fs/ramfs.c
 * ===========================================================================
 *
 * RAM File System (RAMFS) Implementation
 *
 * This file implements a simple, non-persistent file system that lives entirely
 * in kernel heap memory. It demonstrates practical DSA usage:
 *
 * - Trie: Fast path/filename lookup and indexing
 * - N-ary Tree: Directory hierarchy representation
 * - Hash Map: Open file descriptor table for O(1) lookups
 *
 * Features:
 * - File and directory creation/deletion
 * - Read and write operations
 * - Path parsing and traversal
 * - File descriptor management
 * - Reference counting for safe deletion
 *
 * Memory Layout:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │  All data resides in kernel heap (kmalloc/kfree). No disk I/O.          │
 * │  Each file's data is dynamically allocated and can grow on write.       │
 * │  Directory entries link to child inodes via the tree structure.         │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * ===========================================================================
 */

#include "dsa_structures.h"
#include "../memory/memory.h"
#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * External Functions
 * --------------------------------------------------------------------------- */
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);
extern size_t strlen(const char *s);
extern char *strcpy(char *dest, const char *src);
extern int strcmp(const char *s1, const char *s2);
extern char *strncpy(char *dest, const char *src, size_t n);

/* ---------------------------------------------------------------------------
 * Inode Types
 * --------------------------------------------------------------------------- */
typedef enum {
    INODE_TYPE_FILE = 0,        /* Regular file */
    INODE_TYPE_DIRECTORY = 1    /* Directory */
} inode_type_t;

/* ---------------------------------------------------------------------------
 * Inode Structure
 * ---------------------------------------------------------------------------
 * The inode represents a file or directory in the filesystem.
 * It contains metadata and a pointer to the actual data/children.
 * --------------------------------------------------------------------------- */
typedef struct ramfs_inode {
    inode_type_t type;              /* File or directory */
    char name[MAX_FILENAME_LENGTH]; /* Entry name (filename or dirname) */
    size_t size;                    /* Size in bytes (for files) */
    void *data;                     /* File data pointer (for files) */
    size_t capacity;                /* Allocated capacity (for files) */
    uint32_t ref_count;             /* Reference count (open file descriptors) */
    struct ramfs_inode *parent;     /* Parent directory */
    tree_node_t tree_node;          /* Embedded tree node for hierarchy */
    uint32_t inode_number;          /* Unique inode identifier */
} ramfs_inode_t;

/* ---------------------------------------------------------------------------
 * Open File Descriptor Structure
 * ---------------------------------------------------------------------------
 * Represents an open file with current read/write position.
 * --------------------------------------------------------------------------- */
typedef struct {
    int fd;                         /* File descriptor number */
    ramfs_inode_t *inode;           /* Pointer to the inode */
    size_t position;                /* Current read/write position */
    bool read_only;                 /* Read-only flag */
    bool valid;                     /* Is this descriptor valid? */
} open_file_t;

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* Root directory inode */
static ramfs_inode_t *root_inode = NULL;

/* File descriptor table (array-based for simplicity) */
static open_file_t fd_table[MAX_OPEN_FILES];

/* Next available file descriptor */
static int next_fd = 3;  /* 0=stdin, 1=stdout, 2=stderr reserved */

/* Next inode number */
static uint32_t next_inode_number = 1;

/* RAMFS initialization flag */
static bool ramfs_initialized = false;

/* Total bytes used by RAMFS */
static size_t ramfs_total_bytes = 0;

/* ---------------------------------------------------------------------------
 * Forward Declarations
 * --------------------------------------------------------------------------- */
static ramfs_inode_t *create_inode(const char *name, inode_type_t type);
static void destroy_inode(ramfs_inode_t *inode);
static ramfs_inode_t *resolve_path(const char *path);
static ramfs_inode_t *resolve_parent_path(const char *path, char *out_name);
static int allocate_fd(void);
static void free_fd(int fd);
static open_file_t *get_open_file(int fd);
static int compare_inode_name(void *a, void *b);

/* ---------------------------------------------------------------------------
 * Helper: Compare inode names for tree search
 * --------------------------------------------------------------------------- */
static int compare_inode_name(void *a, void *b)
{
    if (a == NULL || b == NULL) return -1;
    ramfs_inode_t *inode_a = (ramfs_inode_t *)a;
    const char *name_b = (const char *)b;
    return strcmp(inode_a->name, name_b);
}

/* ---------------------------------------------------------------------------
 * Helper: Create a new inode
 * --------------------------------------------------------------------------- */
static ramfs_inode_t *create_inode(const char *name, inode_type_t type)
{
    ramfs_inode_t *inode = (ramfs_inode_t *)kmalloc(sizeof(ramfs_inode_t));
    if (inode == NULL) {
        return NULL;
    }

    memset(inode, 0, sizeof(ramfs_inode_t));
    
    inode->type = type;
    inode->size = 0;
    inode->data = NULL;
    inode->capacity = 0;
    inode->ref_count = 0;
    inode->parent = NULL;
    inode->inode_number = next_inode_number++;

    /* Copy name safely */
    if (name != NULL) {
        strncpy(inode->name, name, MAX_FILENAME_LENGTH - 1);
        inode->name[MAX_FILENAME_LENGTH - 1] = '\0';
    } else {
        inode->name[0] = '\0';
    }

    /* Initialize the embedded tree node */
    tree_node_init(&inode->tree_node, inode);

    ramfs_total_bytes += sizeof(ramfs_inode_t);
    return inode;
}

/* ---------------------------------------------------------------------------
 * Helper: Destroy an inode and free resources
 * --------------------------------------------------------------------------- */
static void destroy_inode(ramfs_inode_t *inode)
{
    if (inode == NULL) {
        return;
    }

    /* Don't destroy if still referenced */
    if (inode->ref_count > 0) {
        return;
    }

    /* Free file data if it's a file */
    if (inode->type == INODE_TYPE_FILE && inode->data != NULL) {
        ramfs_total_bytes -= inode->capacity;
        kfree(inode->data);
        inode->data = NULL;
    }

    /* For directories, recursively destroy children */
    if (inode->type == INODE_TYPE_DIRECTORY) {
        tree_node_t *child = inode->tree_node.first_child;
        while (child != NULL) {
            tree_node_t *next = child->next_sibling;
            ramfs_inode_t *child_inode = (ramfs_inode_t *)child->data;
            /* Remove from parent first */
            tree_remove_child(&inode->tree_node, child);
            destroy_inode(child_inode);
            child = next;
        }
    }

    /* Remove from trie index */
    /* Note: Full path would be needed here - simplified for now */

    ramfs_total_bytes -= sizeof(ramfs_inode_t);
    kfree(inode);
}

/* ---------------------------------------------------------------------------
 * Helper: Parse path and find the last component name
 * ---------------------------------------------------------------------------
 * Returns parent directory inode, outputs the final component name.
 * --------------------------------------------------------------------------- */
static ramfs_inode_t *resolve_parent_path(const char *path, char *out_name)
{
    if (path == NULL || out_name == NULL || path[0] != '/') {
        return NULL;
    }

    /* Handle root case */
    if (strcmp(path, "/") == 0) {
        out_name[0] = '\0';
        return root_inode;
    }

    /* Find the last '/' to separate parent path and name */
    const char *last_slash = path;
    const char *ptr = path;
    while (*ptr) {
        if (*ptr == '/') {
            last_slash = ptr;
        }
        ptr++;
    }

    /* Copy the final component name */
    strncpy(out_name, last_slash + 1, MAX_FILENAME_LENGTH - 1);
    out_name[MAX_FILENAME_LENGTH - 1] = '\0';

    /* If it's directly under root */
    if (last_slash == path) {
        return root_inode;
    }

    /* Resolve parent path by traversing */
    ramfs_inode_t *current = root_inode;
    const char *start = path + 1;  /* Skip leading '/' */

    while (start < last_slash) {
        /* Find next component */
        const char *end = start;
        while (end < last_slash && *end != '/') {
            end++;
        }

        /* Extract component name */
        size_t len = end - start;
        if (len == 0) {
            start = end + 1;
            continue;
        }

        char component[MAX_FILENAME_LENGTH];
        if (len >= MAX_FILENAME_LENGTH) {
            len = MAX_FILENAME_LENGTH - 1;
        }
        memcpy(component, start, len);
        component[len] = '\0';

        /* Find child with this name */
        tree_node_t *child_node = tree_find_child(
            &current->tree_node,
            component,
            compare_inode_name
        );

        if (child_node == NULL) {
            return NULL;  /* Path component not found */
        }

        ramfs_inode_t *child_inode = (ramfs_inode_t *)child_node->data;
        if (child_inode->type != INODE_TYPE_DIRECTORY) {
            return NULL;  /* Not a directory */
        }

        current = child_inode;
        start = end + 1;
    }

    return current;
}

/* ---------------------------------------------------------------------------
 * Helper: Resolve a full path to an inode
 * --------------------------------------------------------------------------- */
static ramfs_inode_t *resolve_path(const char *path)
{
    if (path == NULL || path[0] != '/') {
        return NULL;
    }

    /* Handle root */
    if (strcmp(path, "/") == 0) {
        return root_inode;
    }

    /* First try the trie for fast lookup */
    ramfs_inode_t *cached = (ramfs_inode_t *)fs_index_get(path);
    if (cached != NULL) {
        return cached;
    }

    /* Fall back to tree traversal */
    char name[MAX_FILENAME_LENGTH];
    ramfs_inode_t *parent = resolve_parent_path(path, name);
    if (parent == NULL || name[0] == '\0') {
        return NULL;
    }

    /* Find the final component */
    tree_node_t *child_node = tree_find_child(
        &parent->tree_node,
        name,
        compare_inode_name
    );

    if (child_node == NULL) {
        return NULL;
    }

    return (ramfs_inode_t *)child_node->data;
}

/* ---------------------------------------------------------------------------
 * Helper: Allocate a file descriptor
 * --------------------------------------------------------------------------- */
static int allocate_fd(void)
{
    for (int i = 3; i < MAX_OPEN_FILES; i++) {
        if (!fd_table[i].valid) {
            return i;
        }
    }
    return -1;  /* No free descriptors */
}

/* ---------------------------------------------------------------------------
 * Helper: Free a file descriptor
 * --------------------------------------------------------------------------- */
static void free_fd(int fd)
{
    if (fd >= 0 && fd < MAX_OPEN_FILES) {
        fd_table[fd].valid = false;
        fd_table[fd].inode = NULL;
        fd_table[fd].position = 0;
    }
}

/* ---------------------------------------------------------------------------
 * Helper: Get open file structure by fd
 * --------------------------------------------------------------------------- */
static open_file_t *get_open_file(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return NULL;
    }
    if (!fd_table[fd].valid) {
        return NULL;
    }
    return &fd_table[fd];
}

/* ---------------------------------------------------------------------------
 * ramfs_init - Initialize the RAM filesystem
 * ---------------------------------------------------------------------------
 * Creates the root directory and initializes all DSA structures.
 * --------------------------------------------------------------------------- */
void ramfs_init(void)
{
    if (ramfs_initialized) {
        return;
    }

    /* Initialize DSA structures */
    fs_index_init();
    fs_tree_init(NULL);
    file_table_init(MAX_OPEN_FILES);

    /* Initialize file descriptor table */
    memset(fd_table, 0, sizeof(fd_table));
    next_fd = 3;

    /* Create root directory */
    root_inode = create_inode("/", INODE_TYPE_DIRECTORY);
    if (root_inode == NULL) {
        return;  /* Fatal error - cannot initialize filesystem */
    }

    /* Initialize the directory tree with root */
    fs_tree_init(root_inode);

    /* Add root to trie index */
    fs_index_add("/", root_inode);

    ramfs_initialized = true;
}

/* ---------------------------------------------------------------------------
 * ramfs_create - Create a file or directory at the given path
 * ---------------------------------------------------------------------------
 * Parameters:
 *   path - Absolute path to create (e.g., "/home/user/file.txt")
 *   type - INODE_TYPE_FILE or INODE_TYPE_DIRECTORY
 *
 * Returns:
 *   0 on success, -1 on error
 * --------------------------------------------------------------------------- */
int ramfs_create(const char *path, inode_type_t type)
{
    if (!ramfs_initialized || path == NULL || path[0] != '/') {
        return -1;
    }

    /* Check if already exists */
    if (resolve_path(path) != NULL) {
        return -1;  /* Already exists */
    }

    /* Get parent directory and new entry name */
    char name[MAX_FILENAME_LENGTH];
    ramfs_inode_t *parent = resolve_parent_path(path, name);
    if (parent == NULL || name[0] == '\0') {
        return -1;  /* Parent path not found or invalid */
    }

    /* Parent must be a directory */
    if (parent->type != INODE_TYPE_DIRECTORY) {
        return -1;
    }

    /* Create the new inode */
    ramfs_inode_t *new_inode = create_inode(name, type);
    if (new_inode == NULL) {
        return -1;  /* Out of memory */
    }

    /* Link to parent */
    new_inode->parent = parent;
    tree_add_child(&parent->tree_node, &new_inode->tree_node);

    /* Add to trie index for fast lookup */
    fs_index_add(path, new_inode);

    return 0;
}

/* ---------------------------------------------------------------------------
 * ramfs_open - Open a file and return a file descriptor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   path - Absolute path to the file
 *
 * Returns:
 *   File descriptor (>= 0) on success, -1 on error
 * --------------------------------------------------------------------------- */
int ramfs_open(const char *path)
{
    if (!ramfs_initialized || path == NULL) {
        return -1;
    }

    /* Resolve the path to an inode */
    ramfs_inode_t *inode = resolve_path(path);
    if (inode == NULL) {
        return -1;  /* File not found */
    }

    /* Can only open files, not directories */
    if (inode->type != INODE_TYPE_FILE) {
        return -1;
    }

    /* Allocate a file descriptor */
    int fd = allocate_fd();
    if (fd < 0) {
        return -1;  /* No free descriptors */
    }

    /* Initialize the open file structure */
    fd_table[fd].fd = fd;
    fd_table[fd].inode = inode;
    fd_table[fd].position = 0;
    fd_table[fd].read_only = false;
    fd_table[fd].valid = true;

    /* Increment reference count */
    inode->ref_count++;

    /* Add to open file table (hash map) for tracking */
    file_table_add(path, &fd_table[fd]);

    return fd;
}

/* ---------------------------------------------------------------------------
 * ramfs_read - Read data from an open file
 * ---------------------------------------------------------------------------
 * Parameters:
 *   fd     - File descriptor
 *   buffer - Buffer to read into
 *   size   - Number of bytes to read
 *
 * Returns:
 *   Number of bytes read (may be less than size), or -1 on error
 * --------------------------------------------------------------------------- */
ssize_t ramfs_read(int fd, void *buffer, size_t size)
{
    if (!ramfs_initialized || buffer == NULL || size == 0) {
        return -1;
    }

    open_file_t *file = get_open_file(fd);
    if (file == NULL) {
        return -1;  /* Invalid fd */
    }

    ramfs_inode_t *inode = file->inode;
    if (inode == NULL || inode->type != INODE_TYPE_FILE) {
        return -1;
    }

    /* Calculate how much we can read */
    if (file->position >= inode->size) {
        return 0;  /* EOF */
    }

    size_t available = inode->size - file->position;
    size_t to_read = (size < available) ? size : available;

    /* Copy data to buffer */
    if (inode->data != NULL && to_read > 0) {
        memcpy(buffer, (uint8_t *)inode->data + file->position, to_read);
    }

    /* Update position */
    file->position += to_read;

    return (ssize_t)to_read;
}

/* ---------------------------------------------------------------------------
 * ramfs_write - Write data to an open file
 * ---------------------------------------------------------------------------
 * Parameters:
 *   fd     - File descriptor
 *   buffer - Buffer containing data to write
 *   size   - Number of bytes to write
 *
 * Returns:
 *   Number of bytes written, or -1 on error
 * --------------------------------------------------------------------------- */
ssize_t ramfs_write(int fd, const void *buffer, size_t size)
{
    if (!ramfs_initialized || buffer == NULL) {
        return -1;
    }

    if (size == 0) {
        return 0;
    }

    open_file_t *file = get_open_file(fd);
    if (file == NULL) {
        return -1;  /* Invalid fd */
    }

    if (file->read_only) {
        return -1;  /* Cannot write to read-only file */
    }

    ramfs_inode_t *inode = file->inode;
    if (inode == NULL || inode->type != INODE_TYPE_FILE) {
        return -1;
    }

    /* Check RAMFS size limit */
    if (ramfs_total_bytes + size > RAMFS_MAX_SIZE) {
        return -1;  /* Filesystem full */
    }

    /* Calculate required capacity */
    size_t required = file->position + size;

    /* Expand buffer if needed */
    if (required > inode->capacity) {
        /* Allocate with some extra space for future writes */
        size_t new_capacity = required + 256;
        if (new_capacity > RAMFS_MAX_SIZE) {
            new_capacity = required;
        }

        void *new_data = kmalloc(new_capacity);
        if (new_data == NULL) {
            return -1;  /* Out of memory */
        }

        /* Copy existing data */
        if (inode->data != NULL && inode->size > 0) {
            memcpy(new_data, inode->data, inode->size);
            ramfs_total_bytes -= inode->capacity;
            kfree(inode->data);
        }

        inode->data = new_data;
        ramfs_total_bytes += new_capacity;
        inode->capacity = new_capacity;
    }

    /* Write data */
    memcpy((uint8_t *)inode->data + file->position, buffer, size);

    /* Update position and size */
    file->position += size;
    if (file->position > inode->size) {
        inode->size = file->position;
    }

    return (ssize_t)size;
}

/* ---------------------------------------------------------------------------
 * ramfs_close - Close an open file descriptor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   fd - File descriptor to close
 *
 * Returns:
 *   0 on success, -1 on error
 * --------------------------------------------------------------------------- */
int ramfs_close(int fd)
{
    if (!ramfs_initialized) {
        return -1;
    }

    open_file_t *file = get_open_file(fd);
    if (file == NULL) {
        return -1;  /* Invalid fd */
    }

    /* Decrement reference count on inode */
    if (file->inode != NULL && file->inode->ref_count > 0) {
        file->inode->ref_count--;
    }

    /* Free the file descriptor */
    free_fd(fd);

    return 0;
}

/* ---------------------------------------------------------------------------
 * ramfs_unlink - Delete a file or empty directory
 * ---------------------------------------------------------------------------
 * Parameters:
 *   path - Absolute path to the file/directory to delete
 *
 * Returns:
 *   0 on success, -1 on error
 * --------------------------------------------------------------------------- */
int ramfs_unlink(const char *path)
{
    if (!ramfs_initialized || path == NULL) {
        return -1;
    }

    /* Cannot delete root */
    if (strcmp(path, "/") == 0) {
        return -1;
    }

    /* Resolve the path */
    ramfs_inode_t *inode = resolve_path(path);
    if (inode == NULL) {
        return -1;  /* Not found */
    }

    /* Cannot delete if still open */
    if (inode->ref_count > 0) {
        return -1;
    }

    /* Cannot delete non-empty directories */
    if (inode->type == INODE_TYPE_DIRECTORY && 
        inode->tree_node.first_child != NULL) {
        return -1;
    }

    /* Remove from parent's children */
    if (inode->parent != NULL) {
        tree_remove_child(&inode->parent->tree_node, &inode->tree_node);
    }

    /* Remove from trie index */
    fs_index_remove(path);

    /* Destroy the inode */
    destroy_inode(inode);

    return 0;
}

/* ---------------------------------------------------------------------------
 * ramfs_mkdir - Create a directory (convenience wrapper)
 * --------------------------------------------------------------------------- */
int ramfs_mkdir(const char *path)
{
    return ramfs_create(path, INODE_TYPE_DIRECTORY);
}

/* ---------------------------------------------------------------------------
 * ramfs_touch - Create an empty file (convenience wrapper)
 * --------------------------------------------------------------------------- */
int ramfs_touch(const char *path)
{
    return ramfs_create(path, INODE_TYPE_FILE);
}

/* ---------------------------------------------------------------------------
 * ramfs_seek - Set the file position for an open file
 * ---------------------------------------------------------------------------
 * Parameters:
 *   fd       - File descriptor
 *   offset   - Offset from origin
 *   whence   - SEEK_SET (0), SEEK_CUR (1), or SEEK_END (2)
 *
 * Returns:
 *   New position on success, -1 on error
 * --------------------------------------------------------------------------- */
#define RAMFS_SEEK_SET  0
#define RAMFS_SEEK_CUR  1
#define RAMFS_SEEK_END  2

ssize_t ramfs_seek(int fd, ssize_t offset, int whence)
{
    open_file_t *file = get_open_file(fd);
    if (file == NULL) {
        return -1;
    }

    ramfs_inode_t *inode = file->inode;
    if (inode == NULL) {
        return -1;
    }

    ssize_t new_position;
    switch (whence) {
        case RAMFS_SEEK_SET:
            new_position = offset;
            break;
        case RAMFS_SEEK_CUR:
            new_position = (ssize_t)file->position + offset;
            break;
        case RAMFS_SEEK_END:
            new_position = (ssize_t)inode->size + offset;
            break;
        default:
            return -1;
    }

    if (new_position < 0) {
        return -1;
    }

    file->position = (size_t)new_position;
    return new_position;
}

/* ---------------------------------------------------------------------------
 * ramfs_stat - Get file/directory information
 * ---------------------------------------------------------------------------
 * Parameters:
 *   path - Path to the file/directory
 *   size - Output: size of the file (0 for directories)
 *   type - Output: type (INODE_TYPE_FILE or INODE_TYPE_DIRECTORY)
 *
 * Returns:
 *   0 on success, -1 on error
 * --------------------------------------------------------------------------- */
int ramfs_stat(const char *path, size_t *size, inode_type_t *type)
{
    if (!ramfs_initialized || path == NULL) {
        return -1;
    }

    ramfs_inode_t *inode = resolve_path(path);
    if (inode == NULL) {
        return -1;
    }

    if (size != NULL) {
        *size = inode->size;
    }
    if (type != NULL) {
        *type = inode->type;
    }

    return 0;
}

/* ---------------------------------------------------------------------------
 * ramfs_list_dir - List directory contents
 * ---------------------------------------------------------------------------
 * Parameters:
 *   path       - Path to the directory
 *   names      - Output array of name pointers (user provides buffer)
 *   max_entries - Maximum entries to return
 *
 * Returns:
 *   Number of entries found, or -1 on error
 * --------------------------------------------------------------------------- */
int ramfs_list_dir(const char *path, char **names, size_t max_entries)
{
    if (!ramfs_initialized || path == NULL || names == NULL) {
        return -1;
    }

    ramfs_inode_t *inode = resolve_path(path);
    if (inode == NULL) {
        return -1;
    }

    if (inode->type != INODE_TYPE_DIRECTORY) {
        return -1;  /* Not a directory */
    }

    /* Iterate through children */
    size_t count = 0;
    tree_node_t *child = inode->tree_node.first_child;
    
    while (child != NULL && count < max_entries) {
        ramfs_inode_t *child_inode = (ramfs_inode_t *)child->data;
        if (child_inode != NULL) {
            names[count] = child_inode->name;
            count++;
        }
        child = child->next_sibling;
    }

    return (int)count;
}

/* ---------------------------------------------------------------------------
 * ramfs_get_total_bytes - Get total bytes used by RAMFS
 * --------------------------------------------------------------------------- */
size_t ramfs_get_total_bytes(void)
{
    return ramfs_total_bytes;
}

/* ---------------------------------------------------------------------------
 * ramfs_is_initialized - Check if RAMFS is initialized
 * --------------------------------------------------------------------------- */
bool ramfs_is_initialized(void)
{
    return ramfs_initialized;
}
