#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>         // Definition of address structs
#include <netdb.h>              // Definition of servent hostent structs
#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include "../circular_buffer/circular_buffer.h"

#define MAX_LENGTH 10
#define IP_STRING_LENGTH 16

/*--- SUB - ROUTINES USED BY CLIENT FUNCTION ---*/
/* zero(char** A):
      DESCRIPTION:  this function takes as an argument an IP array
                    array and sets each IP[] string to 0.
      INPUT:
                    char **IPaddr: array with IP addresses.*/
void zero( char** A){
  int i;
  for( i = 0; i < MAX_LENGTH; i++)
    memset( A[i], 0, IP_STRING_LENGTH*sizeof(char));
}

/* copy(char **A, char **B, int N):
      DESCRIPTION:  this function takes as an argument two
                    IP arrays and copys IPs from array B to array A.
      INPUT:
                    char **A: array with IP addresses.
                    char **B: array with IP addresses.
                    int N: length of arrays.*/
void copy( char **A, char **B, int N){
  int i;
  for( i = 0; i < N; i++)
    strcpy(A[i], B[i]);
}

/* scan(char **IPaddr):
      DESCRIPTION:  this function is responsible for scanning and finding the connected IPs
                    on the network. To do so it uses command arp -n. The results of the command are stored
                    to IPaddr array. It returns the numeber of the connected devices.
      INPUT:
                    char **IPaddr: empty array to be filled with IP addresses.
      RETURN VALUES:
                    int: -1 on FAILURE, Number of connected devices N on SUCCCESS.
     */
int scan( char **IPaddr){
  FILE *fp;                     // File Descriptor
  char buffer[512];             // buffer to read a line from file descriptor
  char *IP;
  int N;

  N = 0;            // Set Number of connected devices to zero.

  // Scan for devices. Keep on scanning until you find new devices.
  while ( N == 0) {

    // clear arp cache
    system("ip neigh flush all");

    // Broadcast ping !!! Do not Forget to set it correctly
    // system("ping -c10 -b 192.168.0.255 > /dev/null");
    system("ping -c10 -b 10.255.255.255 > /dev/null");
    fflush(stdout);

    fp = popen( "arp -n", "r");           // Execute command arp -n and create a File Descriptor
                                          // which contains the result of the command
    if( fp == NULL){
      printf("CLIENT: ERROR popen command FAILed\n");
      return (-1);  // return error;
    }

    while( fgets( buffer, 512, fp) != NULL){  // Read line by line the new IPs

      if( N > 0){                             // Ignore the first line which includes headers of the columns
        IP = NULL;                            // Keep the string from the begging of the line until
        IP = strtok( buffer, " ");            // the first whitespace character
        //printf("%s\n", IP);

        //int k = strcmp( IP, "192.168.0.5");
        //printf("%d\n", k);
        if( (strcmp( IP, "192.168.0.5") == 0) || (strcmp( IP, "10.0.0.5") == 0) ){  // Ignore PC IP
          continue;
        }
        strcpy( IPaddr[N-1], IP);             // Copy the scanned IP addresss to the IPaddr array
      }

      N++;                                    // Increase the counter
    }                                         // !!! Be aware, the counter equals 1 + Number of connected devices

    if( N != 0)   // If scan discovers a network or more decrease by one to remove the header line.
      N = N - 1;                                // Correct the number of connected devices

    if( pclose(fp) == -1){                    // close file Descriptor
      printf("CLIENT: ERROR pclose command FAILed\n");
      return(-1);     // return error
    }

    if( N == 0){                              // If 0 devices were found. Wait five seconds and try again.
      printf("CLIENT: ACTIVE CONNECTIONS: 0\n");
      sleep(5);
    }

  }

  return N;
}



/* findNewConnections(char **A, char **B, char **C, int N1, int N2):
      DESCRIPTION:  this function compares the old IPs with the new ones and stores the different IP
                    to an array that holds newly connected devices. It returns the number of newly connected devices.
      INPUT:
                    char **A: IP addresses.
                    char **B: previous IP addresses.
                    char **C: new IPs.
                    int N1: length of A.
                    int N2: length of B.
      RETURN VALUES:
                    int: Number of newly connected devices.*/
int findNewConnections( char **A, char **B, char **C, int N1, int N2){
  int Nnew, i, j, newConnection;
  Nnew = 0;

  for( i = 0; i < N1; i++){                    // Compare newly scanned IPs with the IPs from the previous scan.
    newConnection = 1;                        // Set flag to 1

    for( j = 0; j < N2; j++){
      if( strcmp( A[i], B[j]) == 0){          // If the IP already exists in the previous IP array,
        newConnection = 0;                    // then this is an old device. Break the loop and check the next
        break;
      }
    }

    if( newConnection == 1){                  // If no match was found, this is a new device
      strcpy( C[Nnew], A[i]);                 // Then store this ip to New connected devices
      Nnew++;                                 // increase the counter
    }
  }

  return Nnew;
}


/* connect_wait (	int sockno,	struct sockaddr * addr,	size_t addrlen,	struct timeval * timeout):
      DESCRIPTION:  this function implements connection with receive's socket.
                    It implements function connect and additionally tries to connect until timeout occurs.
      INPUT:        It has the same inputs as connect plus timeout.
                    int sockno: socket file descriptor.
                    struct sockaddr * addr: struct to hold socket's info.
                    size_t addrlen: bytes of server address
                    struct timeval * timeout: timeout.
      RETURN VALUES:
                    int: -1 on ERROR
                          0 on SUCCESS
                          1 on TIMEOUT.*/
int connect_wait (	int sockno,	struct sockaddr * addr,	size_t addrlen,	struct timeval * timeout){
	int res, opt;

	// get socket flags
	if ((opt = fcntl (sockno, F_GETFL, NULL)) < 0) {
		return -1;
	}

	// set socket non-blocking
	if (fcntl (sockno, F_SETFL, opt | O_NONBLOCK) < 0) {
		return -1;
	}

	// try to connect
	if ((res = connect (sockno, addr, addrlen)) < 0) {
		if (errno == EINPROGRESS) {
			fd_set wait_set;

			// make file descriptor set with socket
			FD_ZERO (&wait_set);
			FD_SET (sockno, &wait_set);

			// wait for socket to be writable; return after given timeout
			res = select (sockno + 1, NULL, &wait_set, NULL, timeout);
		}
	}
	// connection was successful immediately
	else {
		res = 1;
	}

	// reset socket flags
	if (fcntl (sockno, F_SETFL, opt) < 0) {
		return -1;
	}

	// an error occured in connect or select
	if (res < 0) {
		return -1;
	}
	// select timed out
	else if (res == 0) {
		errno = ETIMEDOUT;
		return 1;
	}
	// almost finished...
	else {
		socklen_t len = sizeof (opt);

		// check for errors in socket layer
		if (getsockopt (sockno, SOL_SOCKET, SO_ERROR, &opt, &len) < 0) {
			return -1;
		}

		// there was an error
		if (opt) {
			errno = opt;
			return -1;
		}
	}

	return 0;
}

/* int checkIPlist( cbuf_handle_t cbuf, char *IP, uint64_t *timestamp):
      DESCRIPTION:  this function scans IP list with previously connected devices to check
                    whether this IP has been met before.
      INPUT:        cbuf_handle_t cbuf: circular buffer container
                    char *IP: IP to check
                    uint64_t *timestamp: variable to return the timestamp of the latest connection.
      RETURN VALUES:
                    int: -1 on NOT found
                          index postion in IP list in SUCCESS.*/
int checkIPlist( cbuf_handle_t cbuf, char *IP, uint64_t *timestamp){
  int flag;
  size_t i;
  uint64_t time;
  char *tempIP = (char*)malloc(16*sizeof(char));

  size_t N = circular_buf_get_top(cbuf);

  flag = 0;                                                   // set flag to not found
  for( i = 0; i < N; i++){                                    // iterate through whole IP list
    circular_buf_read_IP_list( cbuf, tempIP, &time, i);       // read IP at index i

    if( strcmp( IP, tempIP) == 0){                            // Compare read IP with IP
      flag = 1;                                               // If there is a match.
      *timestamp = time;                                      // Copy last connection timestamp
      free(tempIP);                                           // free tempIP
      return ((int)i);                                        // return index postion
    }
  }

  *timestamp = 0;                                             // IP not found
  free(tempIP);                                               // timestamp set explicitly to 0
  return (-1);                                                // index explicitly set to -1 (invalid position)
}


/* size_t set_start_index( cbuf_handle_t cbuf, uint64_t timestamp):
      DESCRIPTION:  this function scans TIMESTAMPS circular buffer to find the more recent messages additions
                    since last connection of connected device.
      INPUT:        cbuf_handle_t cbuf: circular buffer container.
                    uint64_t timestamp: timestamp of addition of the corresponding message.
      RETURN VALUES:
                    size_t: index position of the least recent message than was sent more recent than timestamp connection.
                            if all messages have addition timestamp smaller than connection timestamp, this means tha alla these messages have already been sent.
                            In this case a number greater than capacity is returned.*/
size_t set_start_index( cbuf_handle_t cbuf, uint64_t timestamp){
  size_t i, index, N, capacity;
  uint64_t tempTimestamp;
  char message[277];

  index = circular_buf_get_tail(cbuf);      // tail index
  N = circular_buf_size(cbuf);              // elements in buffer
  i = 0;                                    // iterator
  capacity = circular_buf_capacity(cbuf);   // Buffer capacity

  while( i < N) {                                                   // Seacrh the whole messages Buffer
    circular_buf_read_element( cbuf, &tempTimestamp, message, index);// to finde the messages that were added to the
                                                                    // buffer after the last communication
                                                                    // Then return the index position of that element.
    if( timestamp < tempTimestamp)                                  // The following elements will be sent.
      return index;                                                 // If no timestamp was found alla messages have already been sent.

    i++;
    index = (index + 1) % capacity;
  }

  return (capacity + 1);    // If no newer messages were found since last connection
                            // return a number greater than capacity.
}



/* sendMessage(char *IP, cbuf_handle_t cbuf):
      DESCRIPTION:  this function is responsible for sending Messages to IP address.
      INPUT:
                    char *IP: Receiver's IP.
                    cbuf_handle_t cbuf: circular buffer container.
      RETURN VALUES:
                    int: -1 on FAILURE, 0 on SUCCESS.*/
int sendMessage( char *IP, cbuf_handle_t cbuf){
  // Variables Declaration
  int socketId;
  struct sockaddr_in serverAdress;
  char buffer[277];

  /* Step 1: Create a Socket */
  socketId = socket(  AF_INET, SOCK_STREAM, 0);

  if( socketId < 0){      // If socket creation FAILed return -1
    printf("CLIENT: ERROR in sendMessage(). Socket was not created.\n");
    return(-1);       // return error
  }

  /* Step 2: Initialize Server Info */
  // IMPORTANT: socket adress structures must
  //            always be filled with 0
  memset( &serverAdress, 0, sizeof(serverAdress));
  serverAdress.sin_family = AF_INET;
  serverAdress.sin_port = htons(2288);      // USE PORT 2288
  serverAdress.sin_addr.s_addr = inet_addr(IP);


  /* Step 3: Connect to Server */
  // Variables to hold timeout connection
  struct timeval timeout;                 // Set connection timeout to 2 seconds
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  int status = connect_wait( socketId, (struct sockaddr*) &serverAdress, sizeof(serverAdress), &timeout);     // Attempt to connect to server
  if( status != 0){                                                                                           // 0 is returned on SUCCES
    if(status == -1){                                                                                         // -1 on ERROR
      printf("CLIENT: ERROR in sendMessage(). DID NOT CONNECT TO SERVER. SERVER DOES NOT EXIST ON THE OTHER SIDE.\n");                                 // 1 on TIMEOUT connection
      close(socketId);
      return (-1);      // return error
    }
    else if( status == 1){
      printf("CLIENT: ERROR in sendMessage(). DID NOT CONNECT TO SERVER. TIMEOUT RECEIVED.\n");
      close(socketId);
      return (-1);      // return error
    }
  }


  /* Step 4: Send circular buffer elements */
  uint64_t lastConnectionTimestamp;
  int IPindex = checkIPlist( cbuf, IP, &lastConnectionTimestamp);     // Check if IP has been met before.
                                                                      // -1 on NOT found, Index postion on FOUND
                                                                      // timestamp = 0 on NOT found, else is set to last connection timestamp

  // At this point CIRCULAR BUFFER MESSAGES-TIMESTAMPS are used.
  // Therefore, from now on, no changes can be made on circular buffer.
  // lock circular buffer. DO NOT FORGET TO UNLOCK
  circular_buf_lock( cbuf);

  size_t tail = circular_buf_get_tail(cbuf);              // tail position at circular buffer MESSAGES-TIMESTAMPS
  size_t index;
  size_t N = circular_buf_size(cbuf);                     // Number of elements in circular buffer MESSAGES-TIMESTAMPS
  size_t capacity = circular_buf_capacity(cbuf);          // size of buffer
  size_t head = circular_buf_get_head(cbuf);              // head position at circular buffer MESSAGES-TIMESTAMPS
  struct timeval newTime;                                 // variable to hold new connection timestamp

  if(IPindex < 0){ // THIS IP IS MET FOR THE FIRST TIME SEND ELEMENTS FROM TAIL to HEAD
    printf("CLIENT: FIRST TIME MET IP: %s\n", IP);
    gettimeofday( &newTime, NULL);      // get current timestamp
    circular_buf_put_IP( cbuf, IP, (uint64_t)(newTime.tv_sec)); // Add new IP and its timestamp to IP list and connection timestamp
    IPindex = (int)(circular_buf_get_top(cbuf) - 1);    // Its index in IP list will be at the top - 1 since top always points one position plus the latest element
    index = circular_buf_get_tail(cbuf);                // Since this IP has never met befor send all messages from TAIL to HEAD
  }
  else{ // THIS IP HAS BEEN MET BEFORE. CHECK MESSAGES' ADDITION TIMESTAMP and send only those that have timestamp more recent than latest connection to HEAD
    printf("CLIENT: %s HAS BEEN MET BEFORE WITH TIMESTAMP %" PRIu64 "\n", IP, lastConnectionTimestamp);
    index = set_start_index( cbuf, lastConnectionTimestamp);  // set index to begin sending elements
    gettimeofday( &newTime, NULL);
    circular_buf_set_IP_connection_timestamp( cbuf, (uint64_t)(newTime.tv_sec), (size_t)IPindex); // update connection timestamp
  }

  if( index > capacity){   // If index is greater than capacity this means that alla elements have already been sent to this IP.
    printf("CLIENT: ALL ELEMENTS HAVE ALREADY BEEN SENT\n");
    circular_buf_unlock(cbuf);  // unlock circular buffer is no longer needed.
    gettimeofday( &newTime, NULL);
    circular_buf_set_IP_connection_timestamp( cbuf, (uint64_t)(newTime.tv_sec), (size_t)IPindex); // update connection timestamp
    close(socketId);  // close socket
    return 0;   // return success.
  }
  else{
    if( circular_buf_empty(cbuf)){        // check that there are elements to be sent
      printf("CLIENT: BUFFER EMPTY. NO ELEMENTS TO BE SENT.\n");
      circular_buf_unlock(cbuf);  // unlock circular buffer is no longer needed.
      gettimeofday( &newTime, NULL);
      circular_buf_set_IP_connection_timestamp( cbuf, (uint64_t)(newTime.tv_sec), (size_t)IPindex); // update timestamp
      close(socketId);  // close socket
      return 0;   // return success.
    }

    uint64_t timestamp;                                     // temporary variable to hold a timestamp
    char *message;                                          // temporary variable to hold a message
    message = (char*)malloc(277*sizeof(char));

    size_t Nsends = 0;                                      // counter to track Number of Succesful sends

    timeout.tv_sec = 10;                                     // Set Send Timeout to 1 second
    timeout.tv_usec = 0;
    if ( setsockopt( socketId, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
      perror("setsockopt failed\n");


    struct  timeval start, stop;

    do{                                     // Send circular buffer elements
      memset( message, 0, 277*sizeof(char));

      circular_buf_read_element( cbuf, &timestamp, message, index);   // read message and time stamp at position Nsends
      //printf("%llu\n", timestamp);

      gettimeofday( &start, NULL);                                    // read timestamp before message is sent
      if(  write( socketId, message, 276*sizeof(char)) < 0){          // Send message
        if(errno == EWOULDBLOCK)                                      // Check for Errors
          printf("CLIENT: MESSAGE SEND PROCESS TIMEOUT RECEIVED.\n");         // Timeout received
        else{
          printf("CLIENT: ERROR FAILed TO SEND MESSAGE\n");           // Failed to send messagge
        }
        close(socketId);
        circular_buf_unlock(cbuf);  // unlock circular buffer is no longer needed.
        free(message);
        return ((int)Nsends);
      }
      else{
        gettimeofday( &stop, NULL);
        printf("CLIENT: MESSAGE %s SENT SUCCESSFULLY in %" PRIu64 " usecs.\n", message, (uint64_t)(stop.tv_sec - start.tv_sec)*((uint64_t)(1000000)) + (uint64_t)(stop.tv_usec - start.tv_usec));
      }
      Nsends++;                       // increase the number of successful sends
      index = (index + 1) % capacity;   // increase the index. If index reaches end of buffer restart from 0.
    }while( index != head);

    circular_buf_unlock(cbuf);  // unlock circular buffer is no longer needed.
    free(message);
    close(socketId);
    return((int)Nsends);
  }


  close(socketId);
  return 0;
}
/*-----------------------------------------------------------------------------------*/



/* --- MAIN CLIENT FUNCTION --- */
/*  client( cbuf_handle_t cbuf)
      DESCRIPTION:  this function implements the client in the network.
                    It constantly scans the network for new devices and
                    send them the array with messages.
      INPUT:
                    cbuf_handle_t cbuf: empty circular buffer with messages and timestamps.*/
static void client( cbuf_handle_t cbuf){
  char **IPaddr, **preIPaddr, **newIPaddr;
  int N, Npre, Nnew, i, j;
  struct timeval discoveryTime, startCommunicationTime, stopCommuticationTime;

  // Variables Initialization
  N = 0;              // Number of connected devices to local netowrk.
  Npre = 0;           // Number of connected devices to local network at the previous WiFi scan.

  IPaddr = (char**)malloc(MAX_LENGTH*sizeof(char*));            // Array to store the IP addresses of
  for( i = 0; i < IP_STRING_LENGTH; i++)                        // the devices connected to the local network.
    IPaddr[i] = (char*)malloc(IP_STRING_LENGTH*sizeof(char));

  preIPaddr = (char**)malloc(MAX_LENGTH*sizeof(char*));         // Array to store the IP addresses of
  for( i = 0; i < IP_STRING_LENGTH; i++)                        // the devices that were connected to the
    preIPaddr[i] = (char*)malloc(IP_STRING_LENGTH*sizeof(char));// local network at the previous WiFi scan.

  newIPaddr = (char**)malloc(MAX_LENGTH*sizeof(char*));         // Array to store the IP addresses of
  for( i = 0; i < IP_STRING_LENGTH; i++)                        // the devices that connected now and they
    newIPaddr[i] = (char*)malloc(IP_STRING_LENGTH*sizeof(char));// were not connected at the previous scan.

  zero(IPaddr);           // Set all IPs of array to 0
  zero(preIPaddr);        // Set all IPs of array to 0

  /* CLIENT */
  // This part of program executes an infinite loop
  // scanning for new devices. When a new device appears
  // sends a message.
  while (1) {
    int status;
    //printf("CLIENT: NUMBER OF CONNECTED DEVICES: %d\n", Npre);

    zero(IPaddr);                       // Set all IPs of array to 0

    N = 0;                              // Set N(number of connected devices) to 0
    N = scan(IPaddr);                   // Scan for connected devices

    if( N < 0){
      printf("CLIENT: ERROR on function scan. Retrying....\n");
      continue;
    }
    else if( N >= 0){
      printf("CLIENT: ACTIVE CONNECTIONS: %d\n", N);
    }



    zero(newIPaddr);                    // Set all IPs of array to 0
    Nnew = 0;                           // Set N(number of newly connected devices) to 0
    Nnew = findNewConnections( IPaddr, preIPaddr, newIPaddr, N, Npre);  // Check for newly connected devices

    if( Nnew > 0){
      //printf("CLIENT: NEW CONNECTIONS: %d\n", Nnew);
      gettimeofday( &discoveryTime, NULL);
    }

    for( i = 0; i < Nnew; i++)
      printf("CLIENT: NEW CONNECTION WITH %s  TIMESTAMP: %"PRIu64" secs.\n", newIPaddr[i], (uint64_t)(discoveryTime.tv_sec));

    /* -------- 1st Option ----------- */
    /* The following part of code checks and responds only to new connections ignoring old ones */

    /*for(i = 0; i < Nnew; i++){
      gettimeofday( &startCommunicationTime, NULL);
      status = sendMessage( newIPaddr[i], cbuf);
      if( status >= 0){
        gettimeofday( &stopCommuticationTime, NULL);
        printf("CLIENT: MESSAGES SENT TO %s : %d COMMUNICATION DURATION SINCE SEND PROCESS INITIATED: %"PRIu64"  usecs \n", newIPaddr[i], status, (uint64_t)(stopCommuticationTime.tv_sec - startCommunicationTime.tv_sec)*((uint64_t)(1000000)) + (uint64_t)(stopCommuticationTime.tv_usec - startCommunicationTime.tv_usec));
      }
      else if( status < 0){
        printf("CLIENT: COMMUNICATION TERMINATED UNSUCCESSFULLY WITH IP: %s\n", IPaddr[i]);
      }
    }*/

    /* -------- 2nd Option ----------- */
    /* The following part of code responds to all active connections */
    for(i = 0; i < N; i++){
      gettimeofday( &startCommunicationTime, NULL);
      status = sendMessage( IPaddr[i], cbuf);
      if( status >= 0){
        gettimeofday( &stopCommuticationTime, NULL);
        //printf("CLIENT: COMMUNICATION DURATION SINCE SEND PROCESS INITIATED: %"PRIu64"  usecs \n", (uint64_t)(stopCommuticationTime.tv_sec - startCommunicationTime.tv_sec)*((uint64_t)(1000000)) + (uint64_t)(stopCommuticationTime.tv_usec - startCommunicationTime.tv_usec));
        printf("CLIENT: MESSAGES SENT TO %s : %d\n", IPaddr[i], status);
      }
      else if( status < 0){
        printf("CLIENT: COMMUNICATION TERMINATED UNSUCCESSFULLY WITH IP: %s\n", IPaddr[i]);
      }
    }

    zero(preIPaddr);
    copy( preIPaddr, IPaddr, N);
    Npre = N;

    sleep(5);
  }

  free(IPaddr);
  free(preIPaddr);
  free(newIPaddr);

}
/*------------------------------------------------------------------------------*/

/* --- PTHREAD FUNCTION --- */
/*  *helperClientRoutine(void *cbuf):
      DESCRIPTION:  this function is called by pthread_create and calls the main client function.
      INPUT:
                    void *cbuf: empty circular buffer with messages and timestamps.*/
void *helperClientRoutine(void *cbuf){
  client( (cbuf_handle_t)cbuf);
}
