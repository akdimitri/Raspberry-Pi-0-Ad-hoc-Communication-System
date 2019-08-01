#include <assert.h>     // assert
#include <stdlib.h>     // malloc
#include <stdint.h>     // uint64_t
#include <stdbool.h>    // bool type
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>

#include "circular_buffer.h"

// The hidden definition of our circular buffer structure
struct circular_buf_t {
    uint64_t *timestamp;          // The buffer that holds the timestamp of addition of the corresponding message
    char **message;               // The buffer that holds the messages received.
    size_t head;                  // "head" position (incremented when elements are added)
    size_t tail;                  // "tail" (incremented when elements are removed)
    size_t max;                   // The maximum size of the buffer
    bool full;                    // flag indicating whether the buffer is full or not
    pthread_mutex_t mutex;        // mutex variable to prevent data corruption.



    char **IPlist;                // List to hold IP addresses that have connected at least once at the local network.
    uint64_t *connectionTimestamp;// Last time a connection established with the corresponding IP of the IPlist.
    size_t top;                   // Top of the list. It points at the first empty element of the IP list.
    size_t listSize;                  // IP lists size.
};



/* --- INITIALIZATION FUNCTIONS --- */
/* cbuf_handle_t circular_buf_init( uint64_t *timestamp, char **message, size_t size):
              DESCRIPTION: Initialization of the struct to handle the buffer
                           or essentially create the buffer container.
              INPUTS:
                uint64_t* timestamp: the underlying buffer that contains the timestamps of the messages.
                char **message: the underlying buffer that contains the messages.
                size_t size: the size(length) of the buffer.
              RETURN VALUES:
                cbuf_handle_t: returns a pointer to an initialized circular_buf_t
                               struct.*/
cbuf_handle_t circular_buf_init( uint64_t *timestamp, char **message, size_t size, char **IPlist, uint64_t *connectionTimestamp, size_t listSize){
  assert( timestamp && message && size && IPlist && connectionTimestamp);        // Check that buffer has been allocated and size > 0

  cbuf_handle_t cbuf = malloc(sizeof(circular_buf_t));    // create a struct circular_buf_t pointed
  assert(cbuf);                                           // by cbuf.

  cbuf->timestamp = timestamp;      // Set struct's pointer to timestamp buffer
  cbuf->message = message;          // Set struct's pointer to message buffer
  cbuf->max = size;                 // Set max to size
  cbuf->IPlist = IPlist;            // Set struct's pointer to IPlist array
  cbuf->connectionTimestamp = connectionTimestamp;  // Set struct's pointer to connectionTimestamp array
  cbuf->top = 0;                    // Set top of list at 0.
  cbuf->listSize = listSize;        // Set size of IPlist
  circular_buf_reset(cbuf);         // Reset buffer head tail and full
  pthread_mutex_init( &(cbuf->mutex), NULL);  // Initialize mutex variable.

  assert(circular_buf_empty(cbuf)); // Check that buffer is empty
  return cbuf;
}

/* circular_buf_free(cbuf_handle_t cbuf):
                DESCRIPTION: Destroy the buffer container.

                INPUTS:
                  cbuf_handle_t cbuf: buffer container.*/
void circular_buf_free(cbuf_handle_t cbuf){
    assert(cbuf);     // check tha cbuf exists.
    free(cbuf);
    pthread_mutex_destroy(&(cbuf->mutex));
    // do not attempt to free the underlying buffers,
    // since you do not own it. They should be freed by main.
    // Do Not Forget to free(message); free(timestamp) from main.
}

/* circular_buf_reset(cbuf_handle_t cbuf):
                DESCRIPTION: RESET the buffer and buffer's control registers.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.*/
void circular_buf_reset(cbuf_handle_t cbuf){
    assert(cbuf);

    cbuf->head = 0;
    cbuf->tail = 0;
    cbuf->full = false;
}

/*--------------------------------------------------------------------*/






/*--- STATE FUNCTIONS MESSAGES-ADDITION TIMESTAMP CIRCULAR BUFFER ---*/
/* circular_buf_capacity(cbuf_handle_t cbuf):
                DESCRIPTION: Return the capacity of the buffer.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUES:
                  size_t: returns the capacity of the buffer.*/
size_t circular_buf_capacity(cbuf_handle_t cbuf){
    assert(cbuf);
    return cbuf->max;
}

/* circular_buf_full(cbuf_handle_t cbuf):
                DESCRIPTION: Check that buffer is FULL.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUES:
                  bool: returns TRUE if buffer is FULL, FALSE otherwise.*/
bool circular_buf_full(cbuf_handle_t cbuf){
    assert(cbuf);

    return cbuf->full;
}

/* circular_buf_empty(cbuf_handle_t cbuf):
                DESCRIPTION: Check that buffer is EMPTY.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUES:
                  bool: returns TRUE if EMPTY is FULL, FALSE otherwise.*/
bool circular_buf_empty(cbuf_handle_t cbuf){
    assert(cbuf);

    return (!cbuf->full && (cbuf->head == cbuf->tail));
}


/* circular_buf_size(cbuf_handle_t cbuf):
                DESCRIPTION: finds the Number of the stored items.
                  If the buffer is full, we know that our capacity
                  is at the maximum. If head is greater-than-or-equal-to the tail,
                  we simply subtract the two values to get our size.
                  If tail is greater than head, we need
                  to offset the difference with max to get the correct size.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUES:
                  size_t: the Number of the stored items.*/
size_t circular_buf_size(cbuf_handle_t cbuf){
    assert(cbuf);

    size_t size = cbuf->max;

    if(!cbuf->full){
        if(cbuf->head >= cbuf->tail){
            size = (cbuf->head - cbuf->tail);
        }
        else{
            size = (cbuf->max + cbuf->head - cbuf->tail);
        }
    }

    return size;
}

/* circular_buf_get_tail(cbuf_handle_t cbuf):
                DESCRIPTION: Returns the index of tail position.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUE:
                  size_t: index of tail.*/
size_t circular_buf_get_tail(cbuf_handle_t cbuf){
  assert(cbuf);
  return  cbuf->tail;
}

/* size_t circular_buf_get_head(cbuf_handle_t cbuf):
                DESCRIPTION: Returns the index of head position.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUE:
                  size_t: index of head.*/
size_t circular_buf_get_head(cbuf_handle_t cbuf){
  assert(cbuf);
  return  cbuf->head;
}

/*------------------------------------------------------*/



/*--- ADDITION REMOVAL OF ELEMENTS FUNCTIONS IN MESSAGES-TIMESTAMP CIRCULAR BUFFER ---*/
/* advance_pointer(cbuf_handle_t cbuf):
                DESCRIPTION: helper function that manipulates the head,
                  and tail pointers when adding an element. It also checks
                  whether the buffer is filled up.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.*/
static void advance_pointer(cbuf_handle_t cbuf){
    assert(cbuf);

    if(cbuf->full){                                 // If the buffer is full then
        cbuf->tail = (cbuf->tail + 1) % cbuf->max;  // the tail pointer should be advanced, too.
    }                                               // The modulo(max) ensures that when tail reaches
                                                    // the last element it will be reurned at 0 indexed element
    cbuf->head = (cbuf->head + 1) % cbuf->max;      // Likewise, the head pointer is moved one position.
    cbuf->full = (cbuf->head == cbuf->tail);        // The head pointer points one position greater than the last
}                                                   // added element. Therefore, when head reaches tail, the buffer is full.

/* retreat_pointer(cbuf_handle_t cbuf):
                DESCRIPTION: helper function that manipulates the head,
                  and tail pointers when removing an element. It also checks
                  whether the buffer is filled up.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.*/
static void retreat_pointer(cbuf_handle_t cbuf){
    assert(cbuf);

    cbuf->full = false;
    cbuf->tail = (cbuf->tail + 1) % cbuf->max;
}

/* circular_buf_put(cbuf_handle_t cbuf, uint64_t timestamp, char *message):
                DESCRIPTION: adds an element at the head of buffer.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                  uint64_t timestamp: element to be added.
                  char *message: message to be added.*/
void circular_buf_put(cbuf_handle_t cbuf, uint64_t timestamp, char *message){
    assert(cbuf && cbuf->timestamp && cbuf->message);

    cbuf->timestamp[cbuf->head] = timestamp;           // save timestamp of addition
    strcpy( cbuf->message[cbuf->head], message);       // save message

    advance_pointer(cbuf);
}

/* circular_buf_get(cbuf_handle_t cbuf, uint64_t *timestamp, char *message):
                DESCRIPTION: adds an element at the head of buffer.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                  uint64_t data: value of element to be popped out.
                  char *message: message to be popped out.
                RETURN VALUE:
                  int: 0 on SUCCESS, -1 ON FAILURE.*/
int circular_buf_get(cbuf_handle_t cbuf, uint64_t *timestamp, char *message){
    assert(cbuf && cbuf->timestamp && cbuf->message);

    int r = -1;

    if(!circular_buf_empty(cbuf))    {          // Check if the buffer is not empty.
        *timestamp = cbuf->timestamp[cbuf->tail];
        strcpy( message, cbuf->message[cbuf->tail]);
        retreat_pointer(cbuf);

        r = 0;                                  // return 0 on success
    }

    return r;
}
/*----------------------------------------------------------------------------*/

/* --- MUTEX FUNCTIONS --- */
// In order to prevent multiple locks/unlocks per element. Lock and unlock should be made before/after a process
// is executed. For example, when client decides to send messages. As long as the send process is executed.
// server cannot update circular buffer.
// Race conditions occur only for circular buffer MESSAGES-TIMESTAMPS. No race conditions occur for
// IP list since it's only updated by client.
/* void circular_buf_lock( cbuf_handle_t cbuf):
                DESCRIPTION: lock mutex.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.*/
void circular_buf_lock( cbuf_handle_t cbuf){
  assert(cbuf);
  printf("MUTEX: LOCK\n");

  pthread_mutex_lock(&(cbuf->mutex));
}

/* void circular_buf_unlock( cbuf_handle_t cbuf):
                DESCRIPTION: unlock mutex.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.*/
void circular_buf_unlock( cbuf_handle_t cbuf){
  assert(cbuf);
  printf("MUTEX: UNLOCK\n");
  pthread_mutex_unlock(&(cbuf->mutex));
}
/*-------------------------------------------------------------------------------*/




/*---- READ FUNCTIONS MESSAGES-TIMESTAMPS CIRCULAR BUFFER ----*/
/* circular_buf_print( cbuf_handle_t cbuf):
                DESCRIPTION: prints the elements in buffer.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.*/
void circular_buf_print( cbuf_handle_t cbuf){
  assert(cbuf);                            // check if cbuf exist
  assert(!circular_buf_empty(cbuf));       // if buffer empty exit

  size_t N = circular_buf_size(cbuf);
  int count = 0;
  size_t i = cbuf->tail;
  FILE * fp;

  fp = fopen ("circular_buffer.txt", "w");


  while( count < (int)N) {
    fprintf( fp, "%s \t\t%" PRIu64"\n", cbuf->message[i], cbuf->timestamp[i]);
    i = (i + 1) % (cbuf->max);
    count++;
  }

  fclose(fp);

  printf("CIRCULAR BUFFER PRINTED TO FILE\n");
}

/* circular_buf_read_element( cbuf_handle_t cbuf, uint64_t *timestamp, char **message, int index):
                DESCRIPTION: Returns the element at index position.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                  uint64_t *timestamp:  variable to hold the timestamp value of element at index position.
                  char **message variable to hold the message value of element at index position:
                  int index: element's position at buffer.*/
void circular_buf_read_element( cbuf_handle_t cbuf, uint64_t *timestamp, char *message, size_t index){
  assert(cbuf);
  assert(!circular_buf_empty(cbuf));
  if( cbuf->tail < cbuf->head)                                                          // If tail < head. Accept an index in interval [tail, head)
    assert( (index >= cbuf->tail) && (index < cbuf->head));                             // If head < tail. Accept an index in interval [0, head)OR[tail, max)
  else if( cbuf->tail > cbuf->head){                                                    // If head == tail. Check that buffer is full and procceed with any index in [0, max)
    assert( ((index >= cbuf->tail) && (index < cbuf->max)) || (index >= 0 && index < cbuf->head));
  }
  else{
    assert(circular_buf_full(cbuf));
    assert( (index >= 0) && (index < cbuf->max));
  }

  *timestamp = cbuf->timestamp[index];
  strcpy( message, cbuf->message[index]);

}
/*---------------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/



/* --- IP LIST-CONNECTION TIMESTAMP ARRAY FUNCTIONS ---*/
/* --- READ IP LIST AND CONNECTION TIMESTAMP FUNCTION --- */
/* void circular_buf_read_IP_list( cbuf_handle_t cbuf, char *IP, uint64_t *connectionTimestamp, size_t index):
                DESCRIPTION: Returns the IP abd the connection Timestamp of the element at index position.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                  char *IP: string to return the IP of the element at index position.
                  uint64_t *connectionTimestamp: timestamp to be returned.
                  size_t index: index position at array.*/
void circular_buf_read_IP_list( cbuf_handle_t cbuf, char *IP, uint64_t *connectionTimestamp, size_t index){
  assert(cbuf);     // check cbuf exists.
  assert(IP);       // check IP string exists
  assert(connectionTimestamp);  // check connectionTimestamp pointer exists.
  assert(!circular_buf_IP_list_empty(cbuf));  // check IP list is not empty.
  assert( (index >= 0) && (index < cbuf->top));                 // check index is valid

  strcpy( IP, cbuf->IPlist[index]);                             // copy IP
  *connectionTimestamp = cbuf->connectionTimestamp[index];      // copy timestamp
}
/*-----------------------------------------------------------------------------------*/


/* --- STATE IP LIST AND CONNECTION TIMESTAMP FUNCTIONS --- */
/* bool circular_buf_IP_list_empty( cbuf_handle_t cbuf):
                DESCRIPTION: Returns TRUE/FALSE whether IP list is empty.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUES:
                  bool: TRUE if EMPTY/ FALSE if NOT EMPTY.*/
bool circular_buf_IP_list_empty( cbuf_handle_t cbuf){
  if( cbuf->top > 0)
    return false;
  else
    return true;
}

/* bool circular_buf_IP_list_full( cbuf_handle_t cbuf):
                DESCRIPTION: Returns TRUE/FALSE whether IP list is full.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUES:
                  bool: TRUE if FULL/ FALSE if NOT FULL.*/
bool circular_buf_IP_list_full( cbuf_handle_t cbuf){
  if( cbuf->top == cbuf->listSize)
    return true;
  else
    return false;
}

/* size_t circular_buf_get_top( cbuf_handle_t cbuf):
                DESCRIPTION: Returns the index position of top pointer.
                  Top pointer is always a position ahead of the latest element.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                RETURN VALUES:
                  size_t: index position of top.*/
size_t circular_buf_get_top( cbuf_handle_t cbuf){
  assert(cbuf);

  return cbuf->top;
}
/*------------------------------------------------------------------------------*/

/* ---- IP AND CONNECTION ADDITION AND UPDATE FUNCTIONS --- */
/* void circular_buf_put_IP( cbuf_handle_t cbuf, char *IP, uint64_t timestamp):
                DESCRIPTION: this function adds a new entry (IP, timestamp).
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                  char *IP: IP
                  uint64_t timestamp: connection timestamp.*/
void circular_buf_put_IP( cbuf_handle_t cbuf, char *IP, uint64_t timestamp){
  assert(cbuf && IP);
  assert(!circular_buf_IP_list_full(cbuf));

  strcpy( cbuf->IPlist[cbuf->top], IP);
  cbuf->connectionTimestamp[cbuf->top] = timestamp;

  cbuf->top = cbuf->top + 1;          // increase top
}

/* void circular_buf_set_IP_connection_timestamp( cbuf_handle_t cbuf, uint64_t timestamp, size_t index):
                DESCRIPTION: this function updates the timestamp entry of index position.
                INPUTS:
                  cbuf_handle_t cbuf: buffer container.
                  uint64_t timestamp: connection timestamp.
                  size_t index: index position at array.*/
void circular_buf_set_IP_connection_timestamp( cbuf_handle_t cbuf, uint64_t timestamp, size_t index){
  assert(cbuf);
  assert( (index >= 0) && (index < cbuf->top));

  cbuf->connectionTimestamp[index] = timestamp;
}
