#ifndef CIRCULAR_BUFFER_H_   /* Include guard */
#define CIRCULAR_BUFFER_H_

#include <stdbool.h>
#include <stdint.h>     // uint8_t

// Opaque circular buffer structure
typedef struct circular_buf_t circular_buf_t;

/* We don't want users to work with a circular_buf_t pointer directly,
as they might get the impression that they can dereference the value.
We will create a handle type that they can use instead. */
// Handle type, the way users interact with the API
typedef circular_buf_t* cbuf_handle_t;


/// Pass in a storage buffer and size
/// Returns a circular buffer handle
cbuf_handle_t circular_buf_init( uint64_t *timestamp, char **message, size_t size, char **IPlist, uint64_t *connectionTimestamp, size_t listSize);

/// Free a circular buffer structure.
/// Does not free data buffer; owner is responsible for that
void circular_buf_free(cbuf_handle_t cbuf);

/// Reset the circular buffer to empty, head == tail
void circular_buf_reset(cbuf_handle_t cbuf);

/// Put version 1 continues to add data if the buffer is full
/// Old data is overwritten
void circular_buf_put(cbuf_handle_t cbuf, uint64_t timestamp, char *message);


/// Retrieve a value from the buffer
/// Returns 0 on success, -1 if the buffer is empty
int circular_buf_get(cbuf_handle_t cbuf, uint64_t *timestamp, char *message);

/// Returns true if the buffer is empty
bool circular_buf_empty(cbuf_handle_t cbuf);

/// Returns true if the buffer is full
bool circular_buf_full(cbuf_handle_t cbuf);

/// Returns the maximum capacity of the buffer
size_t circular_buf_capacity(cbuf_handle_t cbuf);

/// Returns the current number of elements in the buffer
size_t circular_buf_size(cbuf_handle_t cbuf);

/// prints the buffer elements from tail to head.
void circular_buf_print( cbuf_handle_t cbuf);

/// read an element at index position
void circular_buf_read_element( cbuf_handle_t cbuf, uint64_t *timestamp, char *message, size_t index);

/// return tail position
size_t circular_buf_get_tail(cbuf_handle_t cbuf);

/// return head position
size_t circular_buf_get_head(cbuf_handle_t cbuf);

/// read an element from IPlist and connectionTimestamp
void circular_buf_read_IP_list( cbuf_handle_t cbuf, char *IP, uint64_t *connectionTimestamp, size_t index);

/// check if IP list is empty
bool circular_buf_IP_list_empty( cbuf_handle_t cbuf);

/// add a new IP to list.
void circular_buf_put_IP( cbuf_handle_t cbuf, char *IP, uint64_t timestamp);

/// return top of IP list
size_t circular_buf_get_top( cbuf_handle_t cbuf);

/// set the connection timestamp of an IP
void circular_buf_set_IP_connection_timestamp( cbuf_handle_t cbuf, uint64_t timestamp, size_t index);

/// lock mutex
void circular_buf_lock( cbuf_handle_t cbuf);

/// unlock mutex
void circular_buf_unlock( cbuf_handle_t cbuf);

#endif
