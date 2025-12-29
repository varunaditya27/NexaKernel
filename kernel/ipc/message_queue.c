/*
 * ===========================================================================
 * kernel/ipc/message_queue.c
 * ===========================================================================
 *
 * IPC Message Queue Implementation
 *
 * This file implements a First-In-First-Out (FIFO) message queue mechanism for
 * Inter-Process Communication (IPC). It allows tasks to safely exchange data
 * messages, facilitating coordination between different parts of the system.
 *
 * Features:
 * - Multiple independent message queues
 * - Variable-size messages with type filtering
 * - Non-blocking and blocking send/receive
 * - Reference counting for safe cleanup
 *
 * DSA Usage:
 * - Circular buffer for message storage within each queue
 *
 * ===========================================================================
 */

#include "../../config/os_config.h"
#include "../memory/memory.h"

/* ---------------------------------------------------------------------------
 * External Functions
 * --------------------------------------------------------------------------- */
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);

/* ---------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------- */
#define MAX_QUEUES          16          /* Maximum number of message queues */
#define MAX_MESSAGES        32          /* Maximum messages per queue */
#define MAX_MSG_SIZE        256         /* Maximum message data size in bytes */

/* ---------------------------------------------------------------------------
 * Message Structure
 * --------------------------------------------------------------------------- */
typedef struct {
    uint32_t sender_pid;                /* Sender task PID */
    uint32_t type;                      /* Message type (for filtering) */
    size_t size;                        /* Actual data size */
    uint8_t data[MAX_MSG_SIZE];         /* Message data buffer */
} message_t;

/* ---------------------------------------------------------------------------
 * Message Queue Structure
 * --------------------------------------------------------------------------- */
typedef struct {
    bool valid;                         /* Is this queue slot in use? */
    uint32_t key;                       /* Queue identifier key */
    uint32_t ref_count;                 /* Reference count */
    
    /* Circular buffer for messages */
    message_t messages[MAX_MESSAGES];
    size_t head;                        /* Next message to read */
    size_t tail;                        /* Next slot to write */
    size_t count;                       /* Current message count */
} msgq_t;

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */
static msgq_t queues[MAX_QUEUES];
static bool msgq_initialized = false;

/* ---------------------------------------------------------------------------
 * msgq_init - Initialize the message queue subsystem
 * --------------------------------------------------------------------------- */
void msgq_init(void)
{
    if (msgq_initialized) {
        return;
    }
    
    /* Clear all queue slots */
    memset(queues, 0, sizeof(queues));
    
    for (size_t i = 0; i < MAX_QUEUES; i++) {
        queues[i].valid = false;
        queues[i].key = 0;
        queues[i].ref_count = 0;
        queues[i].head = 0;
        queues[i].tail = 0;
        queues[i].count = 0;
    }
    
    msgq_initialized = true;
}

/* ---------------------------------------------------------------------------
 * msgq_create - Create a new message queue
 * ---------------------------------------------------------------------------
 * Parameters:
 *   key - Unique identifier for the queue
 *
 * Returns:
 *   Queue ID (>= 0) on success, -1 on error
 * --------------------------------------------------------------------------- */
int msgq_create(uint32_t key)
{
    if (!msgq_initialized) {
        return -1;
    }
    
    /* Check if queue with this key already exists */
    for (size_t i = 0; i < MAX_QUEUES; i++) {
        if (queues[i].valid && queues[i].key == key) {
            /* Return existing queue */
            queues[i].ref_count++;
            return (int)i;
        }
    }
    
    /* Find an empty slot */
    for (size_t i = 0; i < MAX_QUEUES; i++) {
        if (!queues[i].valid) {
            queues[i].valid = true;
            queues[i].key = key;
            queues[i].ref_count = 1;
            queues[i].head = 0;
            queues[i].tail = 0;
            queues[i].count = 0;
            memset(queues[i].messages, 0, sizeof(queues[i].messages));
            return (int)i;
        }
    }
    
    return -1;  /* No free slots */
}

/* ---------------------------------------------------------------------------
 * msgq_get - Get an existing message queue by key
 * ---------------------------------------------------------------------------
 * Parameters:
 *   key - Queue identifier key
 *
 * Returns:
 *   Queue ID (>= 0) on success, -1 if not found
 * --------------------------------------------------------------------------- */
int msgq_get(uint32_t key)
{
    if (!msgq_initialized) {
        return -1;
    }
    
    for (size_t i = 0; i < MAX_QUEUES; i++) {
        if (queues[i].valid && queues[i].key == key) {
            queues[i].ref_count++;
            return (int)i;
        }
    }
    
    return -1;
}

/* ---------------------------------------------------------------------------
 * msgq_destroy - Destroy a message queue
 * ---------------------------------------------------------------------------
 * Parameters:
 *   qid - Queue ID to destroy
 *
 * Returns:
 *   0 on success, -1 on error
 * --------------------------------------------------------------------------- */
int msgq_destroy(int qid)
{
    if (!msgq_initialized || qid < 0 || qid >= MAX_QUEUES) {
        return -1;
    }
    
    msgq_t *q = &queues[qid];
    if (!q->valid) {
        return -1;
    }
    
    /* Decrement reference count */
    if (q->ref_count > 0) {
        q->ref_count--;
    }
    
    /* Only destroy if no references remain */
    if (q->ref_count == 0) {
        q->valid = false;
        q->key = 0;
        q->head = 0;
        q->tail = 0;
        q->count = 0;
    }
    
    return 0;
}

/* ---------------------------------------------------------------------------
 * msgq_send - Send a message to a queue
 * ---------------------------------------------------------------------------
 * Parameters:
 *   qid    - Queue ID
 *   data   - Message data to send
 *   size   - Size of message data
 *   type   - Message type (for filtering on receive)
 *
 * Returns:
 *   0 on success, -1 on error (queue full, invalid params)
 * --------------------------------------------------------------------------- */
int msgq_send(int qid, const void *data, size_t size, uint32_t type)
{
    if (!msgq_initialized || qid < 0 || qid >= MAX_QUEUES) {
        return -1;
    }
    
    if (data == NULL || size == 0 || size > MAX_MSG_SIZE) {
        return -1;
    }
    
    msgq_t *q = &queues[qid];
    if (!q->valid) {
        return -1;
    }
    
    /* Check if queue is full */
    if (q->count >= MAX_MESSAGES) {
        return -1;  /* Queue full - non-blocking */
    }
    
    /* Add message to queue */
    message_t *msg = &q->messages[q->tail];
    msg->sender_pid = 0;  /* TODO: Get current task PID */
    msg->type = type;
    msg->size = size;
    memcpy(msg->data, data, size);
    
    /* Advance tail with wrap-around */
    q->tail = (q->tail + 1) % MAX_MESSAGES;
    q->count++;
    
    return 0;
}

/* ---------------------------------------------------------------------------
 * msgq_receive - Receive a message from a queue
 * ---------------------------------------------------------------------------
 * Parameters:
 *   qid    - Queue ID
 *   buffer - Buffer to receive message data
 *   size   - Size of buffer
 *   type   - Message type filter (0 = any type)
 *
 * Returns:
 *   Size of received message (> 0) on success, 0 if no message, -1 on error
 * --------------------------------------------------------------------------- */
ssize_t msgq_receive(int qid, void *buffer, size_t size, uint32_t type)
{
    if (!msgq_initialized || qid < 0 || qid >= MAX_QUEUES) {
        return -1;
    }
    
    if (buffer == NULL || size == 0) {
        return -1;
    }
    
    msgq_t *q = &queues[qid];
    if (!q->valid) {
        return -1;
    }
    
    /* Check if queue is empty */
    if (q->count == 0) {
        return 0;  /* No messages - non-blocking */
    }
    
    /* If type filter specified, search for matching message */
    if (type != 0) {
        size_t idx = q->head;
        for (size_t i = 0; i < q->count; i++) {
            if (q->messages[idx].type == type) {
                /* Found matching message - copy it */
                message_t *msg = &q->messages[idx];
                size_t copy_size = (msg->size < size) ? msg->size : size;
                memcpy(buffer, msg->data, copy_size);
                
                /* Remove message by shifting (simple approach) */
                /* Note: In production, use a more efficient removal */
                for (size_t j = idx; j != q->tail; ) {
                    size_t next = (j + 1) % MAX_MESSAGES;
                    if (next == q->tail) break;
                    q->messages[j] = q->messages[next];
                    j = next;
                }
                q->tail = (q->tail + MAX_MESSAGES - 1) % MAX_MESSAGES;
                q->count--;
                
                return (ssize_t)copy_size;
            }
            idx = (idx + 1) % MAX_MESSAGES;
        }
        return 0;  /* No matching message */
    }
    
    /* No type filter - get message from head (FIFO) */
    message_t *msg = &q->messages[q->head];
    size_t copy_size = (msg->size < size) ? msg->size : size;
    memcpy(buffer, msg->data, copy_size);
    
    /* Advance head with wrap-around */
    q->head = (q->head + 1) % MAX_MESSAGES;
    q->count--;
    
    return (ssize_t)copy_size;
}

/* ---------------------------------------------------------------------------
 * msgq_peek - Check if messages are available without removing
 * ---------------------------------------------------------------------------
 * Parameters:
 *   qid - Queue ID
 *
 * Returns:
 *   Number of messages in queue, or -1 on error
 * --------------------------------------------------------------------------- */
int msgq_peek(int qid)
{
    if (!msgq_initialized || qid < 0 || qid >= MAX_QUEUES) {
        return -1;
    }
    
    msgq_t *q = &queues[qid];
    if (!q->valid) {
        return -1;
    }
    
    return (int)q->count;
}

/* ---------------------------------------------------------------------------
 * msgq_count - Get number of active message queues
 * --------------------------------------------------------------------------- */
size_t msgq_count(void)
{
    if (!msgq_initialized) {
        return 0;
    }
    
    size_t count = 0;
    for (size_t i = 0; i < MAX_QUEUES; i++) {
        if (queues[i].valid) {
            count++;
        }
    }
    return count;
}

/* ---------------------------------------------------------------------------
 * msgq_is_initialized - Check if subsystem is initialized
 * --------------------------------------------------------------------------- */
bool msgq_is_initialized(void)
{
    return msgq_initialized;
}

/* Legacy compatibility wrapper */
void ipc_queue_init(void)
{
    msgq_init();
}
