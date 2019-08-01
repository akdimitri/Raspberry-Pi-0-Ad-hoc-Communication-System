/*  Author:     Dimitrios Antoniadis
*    email:      akdimitri@auth.gr
*    University: Aristotle University of Thessaloniki (AUTH)
*    Subject:    Real Time Embedded Systems
*    Semester:   Semester
*
*    Description:  This is the implementation of the final project in the context of the
*                  subject "Real Time Embedded Systems". It implements a communication
*                  system using an ad-hoc netwrok. implementation is aimed for raspberry pi
*                  zero devices. Messages are exchanged through devices until they reach their
*                  destination.
**************************************************************************************************/
/* arm-linux-gnueabihf-gcc main.c ./server/server.c ./client/client.c ./message_generator/message_generator.c ./circular_buffer/circular_buffer.c -o main -march=armv6 -mfloat-abi=hard -mfpu=vfp -pthread */


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include "./server/server.h"
#include "./client/client.h"
#include "./circular_buffer/circular_buffer.h"
#include "./message_generator/message_generator.h"

#define BUFFER_SIZE 2000           // Circular Buffer Size.
#define MESSAGE_LENGTH 277      // Message length of 277 characters.
#define ARRAY_SIZE 50           // Array to hold IPs of the connected Devices.


int main(int argc, char const *argv[]) {
  /*if( argc != 2){
    printf("USAGE: >> %s TIME_OF_EXECUTION_IN_HOURS\n", argv[0]);
    exit(EXIT_FAILURE);
  }*/

  size_t i;
  uint64_t time;
  struct timeval tv;

  // Circular Buffer Initialization
  uint64_t  *timestamp = (uint64_t*)malloc( BUFFER_SIZE * sizeof(uint64_t));          // Timestamp circular buffer will hold the entry timestamp of the relevant message.

  char **message = (char**)malloc( BUFFER_SIZE * sizeof(char*));                      // Message circular buffer wil hold the messages that have been received
  for(i = 0; i < BUFFER_SIZE; i++)
    message[i] = (char*)malloc( MESSAGE_LENGTH * sizeof(char));

  char **IPlist = (char**)malloc( ARRAY_SIZE * sizeof(char*));                        // IP list holds the IPs of the devices that have been connected previously.
  for(i = 0; i < ARRAY_SIZE; i++)
    IPlist[i] = (char*)malloc( 16 * sizeof(char));

  uint64_t *connectionTimestamp = (uint64_t*)malloc( ARRAY_SIZE*sizeof(uint64_t));    // Timestamp of the latest connection established with the corresponding IP from the IP list.

  // Initialize circular buffer
  cbuf_handle_t cbuf = circular_buf_init( timestamp, message, BUFFER_SIZE, IPlist, connectionTimestamp, ARRAY_SIZE);           // Initialize Circular Buffer

  // --------------------------------------------- //
  // Add a message to the buffer manually
  // gettimeofday( &tv, NULL);
  // time = (uint64_t)tv.tv_sec;
  // circular_buf_put( cbuf, time, "8901_8356_1554529172_CPU temperature is : 56.700000");

  // Enter an IP in the IP list manually
  // gettimeofday( &tv, NULL);
  // time = (uint64_t)tv.tv_sec;
  // circular_buf_put_IP( cbuf, "10.0.0.5", time);
  //---------------------------------------------------------//


  // Threads Initialization
  pthread_t messageGeneratorThread;         // thread initialization to hold the messages generator
  if( pthread_create( &messageGeneratorThread, NULL, (void *)helperMessageGeneratorRoutine, (void *)cbuf)){
    printf("ERROR: Could not create MESSAGES GENERATOR thread.Exiting...\n");
    exit(EXIT_FAILURE);
  }
  
  pthread_t serverThread;         // thread initialization to hold the server
  if( pthread_create( &serverThread, NULL, (void *)helperServerRoutine, (void *)cbuf)){
    printf("ERROR: Could not create SERVER thread.Exiting...\n");
    exit(EXIT_FAILURE);
  }

  pthread_t clientThread;         // thread initialization to hold the client
  if( pthread_create( &clientThread, NULL, (void *)helperClientRoutine, (void *)cbuf)){
    printf("ERROR: Could not create SERVER thread.Exiting...\n");
    exit(EXIT_FAILURE);
  }



  // Execution time
  //float executionDuration = atof(argv[1])*3600;
  //sleep((int)executionDuration); // sleep main thread

  // wake up - kill client/server/messagecreator threads
  //pthread_cancel(serverThread);
  //pthread_cancel(clientThread);
  //pthread_cancel(messageGeneratorThread);
  pthread_exit(NULL);


  //printf("\n\n\nCIRCULAR BUFFER\nMESSAGES \t TIMESTAMPS\n");
  //circular_buf_print(cbuf);

  // clean up
  circular_buf_free(cbuf);
  free(timestamp);
  free(message);
  free(IPlist);
  free(connectionTimestamp);
  return 0;
}
