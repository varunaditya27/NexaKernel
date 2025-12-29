/*
 * ===========================================================================
 * kernel/ipc/shared_memory.c
 * ===========================================================================
 *
 * IPC Shared Memory Implementation
 *
 * This file implements shared memory regions for Inter-Process Communication.
 * It allows multiple tasks to access the same physical memory region, enabling
 * high-speed, zero-copy data exchange between processes.
 *
 * Features:
 * - Create and destroy shared memory segments
 * - Attach/detach regions to tasks
 * - Reference counting for safe cleanup
 * - Key-based identification
 *
 * Memory is allocated from the kernel heap. In a more advanced implementation,
 * this would use virtual memory mapping to share physical pages between
 * address spaces.
 *
 * ===========================================================================
 */

#include "../../config/os_config.h"
#include "../memory/memory.h"

/* ---------------------------------------------------------------------------
 * External Functions
 * --------------------------------------------------------------------------- */
extern void *memset(void *s, int c, size_t n);

/* ---------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------- */
#define MAX_SHM_REGIONS     16          /* Maximum number of shared memory regions */
#define MAX_ATTACHMENTS     64          /* Maximum total attachments */
#define SHM_MIN_SIZE        64          /* Minimum region size */
#define SHM_MAX_SIZE        (1024*1024) /* Maximum region size (1MB) */

/* ---------------------------------------------------------------------------
 * Shared Memory Region Structure
 * --------------------------------------------------------------------------- */
typedef struct {
    bool valid;                         /* Is this slot in use? */
    uint32_t key;                       /* Region identifier key */
    void *data;                         /* Pointer to allocated memory */
    size_t size;                        /* Size of region in bytes */
    uint32_t ref_count;                 /* Number of current attachments */
    uint32_t creator_pid;               /* PID of creating task */
} shm_region_t;

/* ---------------------------------------------------------------------------
 * Attachment Tracking Structure
 * ---------------------------------------------------------------------------
 * Tracks which tasks have attached to which shared memory regions.
 * --------------------------------------------------------------------------- */
typedef struct {
    bool valid;                         /* Is this attachment active? */
    int shm_id;                         /* Shared memory region ID */
    uint32_t task_pid;                  /* Task that attached */
    void *attached_addr;                /* Address where attached */
} shm_attachment_t;

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */
static shm_region_t shm_regions[MAX_SHM_REGIONS];
static shm_attachment_t attachments[MAX_ATTACHMENTS];
static bool shm_initialized = false;

/* ---------------------------------------------------------------------------
 * shm_init - Initialize the shared memory subsystem
 * --------------------------------------------------------------------------- */
void shm_init(void)
{
    if (shm_initialized) {
        return;
    }
    
    /* Clear all region slots */
    memset(shm_regions, 0, sizeof(shm_regions));
    for (size_t i = 0; i < MAX_SHM_REGIONS; i++) {
        shm_regions[i].valid = false;
        shm_regions[i].key = 0;
        shm_regions[i].data = NULL;
        shm_regions[i].size = 0;
        shm_regions[i].ref_count = 0;
        shm_regions[i].creator_pid = 0;
    }
    
    /* Clear all attachment slots */
    memset(attachments, 0, sizeof(attachments));
    for (size_t i = 0; i < MAX_ATTACHMENTS; i++) {
        attachments[i].valid = false;
    }
    
    shm_initialized = true;
}

/* ---------------------------------------------------------------------------
 * shm_create - Create a new shared memory region
 * ---------------------------------------------------------------------------
 * Parameters:
 *   key  - Unique identifier for the region
 *   size - Size of the region in bytes
 *
 * Returns:
 *   Shared memory ID (>= 0) on success, -1 on error
 * --------------------------------------------------------------------------- */
int shm_create(uint32_t key, size_t size)
{
    if (!shm_initialized) {
        return -1;
    }
    
    /* Validate size */
    if (size < SHM_MIN_SIZE || size > SHM_MAX_SIZE) {
        return -1;
    }
    
    /* Check if region with this key already exists */
    for (size_t i = 0; i < MAX_SHM_REGIONS; i++) {
        if (shm_regions[i].valid && shm_regions[i].key == key) {
            /* Return existing region if size matches */
            if (shm_regions[i].size >= size) {
                return (int)i;
            }
            return -1;  /* Size mismatch */
        }
    }
    
    /* Find an empty slot */
    int slot = -1;
    for (size_t i = 0; i < MAX_SHM_REGIONS; i++) {
        if (!shm_regions[i].valid) {
            slot = (int)i;
            break;
        }
    }
    
    if (slot < 0) {
        return -1;  /* No free slots */
    }
    
    /* Align size to page boundary for efficiency */
    size_t aligned_size = ALIGN_UP(size, 64);
    
    /* Allocate memory */
    void *data = kmalloc(aligned_size);
    if (data == NULL) {
        return -1;  /* Out of memory */
    }
    
    /* Initialize the memory to zero */
    memset(data, 0, aligned_size);
    
    /* Initialize the region */
    shm_regions[slot].valid = true;
    shm_regions[slot].key = key;
    shm_regions[slot].data = data;
    shm_regions[slot].size = aligned_size;
    shm_regions[slot].ref_count = 0;
    shm_regions[slot].creator_pid = 0;  /* TODO: Get current task PID */
    
    return slot;
}

/* ---------------------------------------------------------------------------
 * shm_get - Get an existing shared memory region by key
 * ---------------------------------------------------------------------------
 * Parameters:
 *   key - Region identifier key
 *
 * Returns:
 *   Shared memory ID (>= 0) on success, -1 if not found
 * --------------------------------------------------------------------------- */
int shm_get(uint32_t key)
{
    if (!shm_initialized) {
        return -1;
    }
    
    for (size_t i = 0; i < MAX_SHM_REGIONS; i++) {
        if (shm_regions[i].valid && shm_regions[i].key == key) {
            return (int)i;
        }
    }
    
    return -1;
}

/* ---------------------------------------------------------------------------
 * shm_attach - Attach to a shared memory region
 * ---------------------------------------------------------------------------
 * Parameters:
 *   shm_id - Shared memory ID
 *   addr   - Preferred address (NULL for any) - currently ignored
 *
 * Returns:
 *   Pointer to shared memory on success, NULL on error
 *
 * Note: In a full virtual memory implementation, this would map the physical
 * pages into the calling task's address space. In this simplified version,
 * we return the kernel heap pointer directly (works for kernel tasks).
 * --------------------------------------------------------------------------- */
void *shm_attach(int shm_id, void *addr)
{
    UNUSED(addr);  /* Address hint not used in this implementation */
    
    if (!shm_initialized || shm_id < 0 || shm_id >= MAX_SHM_REGIONS) {
        return NULL;
    }
    
    shm_region_t *region = &shm_regions[shm_id];
    if (!region->valid || region->data == NULL) {
        return NULL;
    }
    
    /* Find an attachment slot */
    int attach_slot = -1;
    for (size_t i = 0; i < MAX_ATTACHMENTS; i++) {
        if (!attachments[i].valid) {
            attach_slot = (int)i;
            break;
        }
    }
    
    if (attach_slot < 0) {
        return NULL;  /* No attachment slots */
    }
    
    /* Record the attachment */
    attachments[attach_slot].valid = true;
    attachments[attach_slot].shm_id = shm_id;
    attachments[attach_slot].task_pid = 0;  /* TODO: Get current task PID */
    attachments[attach_slot].attached_addr = region->data;
    
    /* Increment reference count */
    region->ref_count++;
    
    return region->data;
}

/* ---------------------------------------------------------------------------
 * shm_detach - Detach from a shared memory region
 * ---------------------------------------------------------------------------
 * Parameters:
 *   addr - Address previously returned by shm_attach()
 *
 * Returns:
 *   0 on success, -1 on error
 * --------------------------------------------------------------------------- */
int shm_detach(void *addr)
{
    if (!shm_initialized || addr == NULL) {
        return -1;
    }
    
    /* Find the attachment by address */
    for (size_t i = 0; i < MAX_ATTACHMENTS; i++) {
        if (attachments[i].valid && attachments[i].attached_addr == addr) {
            int shm_id = attachments[i].shm_id;
            
            /* Clear the attachment */
            attachments[i].valid = false;
            attachments[i].shm_id = 0;
            attachments[i].task_pid = 0;
            attachments[i].attached_addr = NULL;
            
            /* Decrement reference count */
            if (shm_id >= 0 && shm_id < MAX_SHM_REGIONS) {
                if (shm_regions[shm_id].ref_count > 0) {
                    shm_regions[shm_id].ref_count--;
                }
            }
            
            return 0;
        }
    }
    
    return -1;  /* Attachment not found */
}

/* ---------------------------------------------------------------------------
 * shm_destroy - Destroy a shared memory region
 * ---------------------------------------------------------------------------
 * Parameters:
 *   shm_id - Shared memory ID
 *
 * Returns:
 *   0 on success, -1 on error
 *
 * Note: Region is only destroyed if ref_count is 0 (no attachments).
 * --------------------------------------------------------------------------- */
int shm_destroy(int shm_id)
{
    if (!shm_initialized || shm_id < 0 || shm_id >= MAX_SHM_REGIONS) {
        return -1;
    }
    
    shm_region_t *region = &shm_regions[shm_id];
    if (!region->valid) {
        return -1;
    }
    
    /* Cannot destroy if there are active attachments */
    if (region->ref_count > 0) {
        return -1;
    }
    
    /* Free the memory */
    if (region->data != NULL) {
        kfree(region->data);
        region->data = NULL;
    }
    
    /* Clear the region */
    region->valid = false;
    region->key = 0;
    region->size = 0;
    region->ref_count = 0;
    region->creator_pid = 0;
    
    return 0;
}

/* ---------------------------------------------------------------------------
 * shm_get_size - Get the size of a shared memory region
 * ---------------------------------------------------------------------------
 * Parameters:
 *   shm_id - Shared memory ID
 *
 * Returns:
 *   Size in bytes, or 0 on error
 * --------------------------------------------------------------------------- */
size_t shm_get_size(int shm_id)
{
    if (!shm_initialized || shm_id < 0 || shm_id >= MAX_SHM_REGIONS) {
        return 0;
    }
    
    shm_region_t *region = &shm_regions[shm_id];
    if (!region->valid) {
        return 0;
    }
    
    return region->size;
}

/* ---------------------------------------------------------------------------
 * shm_count - Get number of active shared memory regions
 * --------------------------------------------------------------------------- */
size_t shm_count(void)
{
    if (!shm_initialized) {
        return 0;
    }
    
    size_t count = 0;
    for (size_t i = 0; i < MAX_SHM_REGIONS; i++) {
        if (shm_regions[i].valid) {
            count++;
        }
    }
    return count;
}

/* ---------------------------------------------------------------------------
 * shm_is_initialized - Check if subsystem is initialized
 * --------------------------------------------------------------------------- */
bool shm_is_initialized(void)
{
    return shm_initialized;
}

/* Legacy compatibility wrapper */
void shared_memory_init(void)
{
    shm_init();
}
