#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>         // Definition of address structs
#include <netdb.h>              // Definition of servent hostent structs
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <inttypes.h>
#include "../circular_buffer/circular_buffer.h"

const char myAEM[] = "8535";

/* int checkForDuplicate( cbuf_handle_t cbuf, char *buffer):
      DESCRIPTION:  this function checks if message in buffer has already been saved
                    from previous communication.
      INPUT:
                    void *cbuf: empty circular buffer with messages and timestamps.*/
int checkForDuplicate( cbuf_handle_t cbuf, char *buffer){
  size_t index, i, size;
  uint64_t timestamp;
  char message[277];
  int found = 1;
  char tempBuffer[277];

  strcpy( tempBuffer, buffer);

  index = circular_buf_get_tail(cbuf);  // start iteration from tail
  size = circular_buf_size(cbuf);       // read Number of elements in buffer

  i = 0;                                // iterate untill i reaches size
  while (i < size) {                    // iterate through all elements of buffer.

    circular_buf_read_element( cbuf, &timestamp, message, index); //read element at index position

    if( strcmp(tempBuffer, message) == 0){  // if buffer == message return negative
      found = 0;                        // Message already exists
      return -1;
    }
                                        // Seacrh next.
    index++;                            // increase index
    i++;                                // increase i
  }

  if( found == 1){
    return 1;
  }
  else{
    return -1;
  }
}

/* int amIrecipient( char *buffer):
      DESCRIPTION:  this function checks if I am the recipient of the message.
      INPUT:
                    char *buffer: message.
      RETURN VALUES:
                    int: 1 on match
                         -1 else.*/
int amIrecipient( char *buffer){
  char *recipient;
  char tempBuffer[277];
  strcpy( tempBuffer, buffer);
  recipient = strtok( tempBuffer, "_");
  recipient = strtok( NULL, "_");

  if( strcmp( recipient, myAEM) == 0){
    return 1;
  }

  return -1;
}


/* static void server( cbuf_handle_t cbuf):
      DESCRIPTION:  this function implements the server
                    of the embedded system. It is constantly waiting for a new
                    communication.
      INPUT:
                    void *cbuf: empty circular buffer with messages and timestamps.*/
static void server( cbuf_handle_t cbuf){
  int socketId, clientLength, Nrecv, Nsaved, Nsuccess;
  struct sockaddr_in serverAdress, clientAdress;
  char buffer[277];
  char IP[16];
  uint64_t time;
  struct timeval timestamp;

  /* Step 1: Create a Socket and Initialize it*/
  socketId = socket(  AF_INET, SOCK_STREAM, 0);

  if( socketId < 0){
    printf("SERVER: ERROR SOCKET WAS NOT CREATED.\nExiting....\n");
    exit(EXIT_FAILURE);
  }

  // IMPORTANT: socket adress structures nust
  //            always be filled with 0
  memset( &serverAdress, 0, sizeof(serverAdress));

  serverAdress.sin_family = AF_INET;
  serverAdress.sin_port = htons(2288);      // USE PORT 2288
  serverAdress.sin_addr.s_addr = INADDR_ANY;

  /* Step 2: Bind the Socket */
  if( bind( socketId, (struct sockaddr*) &serverAdress,  sizeof(serverAdress))){
    printf("SERVER: ERROR SOCKET COULD NOT BE BINDED.\nExiting....\n");
    exit(EXIT_FAILURE);
  }

  /* Step 3: Listen to socket */           // IMPORTANT!!: SET MAX MAX_LENGTH OF client.c the same amount as below, in order to accept as many connected devices as a server can handle.
  if( listen( socketId, 10) < 0){          // 10 means that in qeue at most 10 users can wait
    printf("SERVER: ERROR SOCKET COULD NOT BE SET AS PASSIVE SOCKET\nExiting....\n");
    exit(EXIT_FAILURE);
  }

  /* On this state server waits until someone request to communicate with server */

  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  /* Step 4: Accept */
  clientLength = sizeof(clientAdress);

  while(1){
    fflush(stdout);
    // wait untill a new connection request to communicate.
    // Initialize incoming socekt info.
    int incomingSocketId = accept( socketId, (struct sockaddr*) &clientAdress, &clientLength);

    if( incomingSocketId < 0){  // if incoming socket initialization failed continue the infinite loop
      printf("SERVER: ERROR REQUEST FOR COMMUNICATION WAS NOT ACCEPTED.\n");
      continue;
    }

    // Set timeout on received messages. Variable struct timeval timeout is responsible for defining the timeout.
    if ( setsockopt( incomingSocketId, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
      printf("SERVER: ERROR TIMEOUT SET FAILED\n");
      continue;
    }

    inet_ntop( AF_INET, &(clientAdress.sin_addr), IP, INET_ADDRSTRLEN );
    printf("SERVER: %s GRANTED COMMUNICATION\n", IP);

    // Read incoming messages.
    // On this state Communication channel has been created.
    // Therefore, server must lock access to circular buffer.
    circular_buf_lock(cbuf);    // DO NOT FORGET TO UNLOCK

    Nsuccess = 0;
    Nrecv = 0;
    Nsaved = 0;
    while(1){
      memset( buffer, 0, 277*sizeof(char));
      int rc = read( incomingSocketId, &buffer, 276*sizeof(char));

      if( rc < 0){
        if(errno == EWOULDBLOCK)
          printf("SERVER: TIMEOUT RECEIVED\n");
        else
          printf("SERVER: ERROR COULD NOT READ MESSAGE FROM CLIENT\n");

        close(incomingSocketId);  // close communication channel.
        break;                    // break communication
      }
      else if( rc == 0){
        printf("SERVER: SOCKET HAS BEEN CLOSED SUCCESSFULLY\n");
        close(incomingSocketId);  // close communication channel.
        break;                    // break communication
      }
      else{
        printf("SERVER: MESSAGE RECEIVED: %s\n", buffer);
        if( checkForDuplicate( cbuf, buffer) > 0){
          printf("SERVER: NEW MESSAGE RECEIVED\n");
          if( amIrecipient(buffer) > 0){
            printf("SERVER: NEW MESSAGE ARRIVED AT ITS DESTINATION\n");
            Nsuccess++;
          }
          else{
            gettimeofday( &timestamp, NULL);
            time = (uint64_t)timestamp.tv_sec;
            circular_buf_put( cbuf, time,  buffer);
            Nsaved++;
          }
          Nrecv++;
        }
        else{
          printf("SERVER: MESSAGE HAS ALREADY BEEN MET\n");
          Nrecv++;
        }
      }
    }

    // Coummunication has been completed.
    circular_buf_print(cbuf);   // print circular buffer to file
    circular_buf_unlock(cbuf);  // unlock circular buffer.
    printf("SERVER: MESSAGES RECEIVED SUCCESSFULLY FROM %s: %d. MESSAGES SAVED: %d\n",  IP, Nrecv, Nsaved);
  }

  close(socketId);
  return;
}

/*------------------------------------------------------------------------------*/

/* --- PTHREAD FUNCTION --- */
/* void *helperServerRoutine(void *cbuf):
      DESCRIPTION:  this function is called by pthread_create and calls the main server function.
      INPUT:
                    void *cbuf: empty circular buffer with messages and timestamps.*/
void *helperServerRoutine(void *cbuf){
  server( (cbuf_handle_t)cbuf);
}
