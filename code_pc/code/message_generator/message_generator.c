#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>     // uint64_t
#include <inttypes.h>
#include "../circular_buffer/circular_buffer.h"

#define AEMSIZE 100

static void messageGenerator( cbuf_handle_t cbuf){
  int r;
  char buffer[277];
  char tempBuf[6];
  uint64_t time_u;
  uint32_t myAEM = 8535;
  uint32_t toAEM;
  struct timeval timestamp;

  srand(time(NULL));   // Initialization of random seed

  /* Uncomment following lines if you are going to use OPTION 2 */
  /* Also Uncomment #define AEMSIZE XXX */
  int *AEMLIST = (int*)malloc(AEMSIZE*sizeof(int));
  int N_AEM = 0;
  FILE *fid;
  fid = fopen( "./AEM.txt", "r");
  if(fid < 0){
    printf("MESSAGE GENERATOR: ERROR on opening AEM list file...Exiting\n", );
  }

  while( fscanf( fid, "%d\n", &AEMLIST[N_AEM]) != EOF){
    N_AEM++;
  }

  fclose(fid);

  /*for( int i = 0 ; i < N_AEM; i++)
    printf("%d\n", AEMLIST[i]);*/
  /*-------------------------------------------------------------*/


  while(1){
    // New Message Creation
    // Lock circular buffer
    circular_buf_lock( cbuf);

    // read temperature
    FILE *temperatureFile;
    double T;
    temperatureFile = popen ("cat /sys/class/thermal/thermal_zone*/temp", "r");

    if (temperatureFile == NULL){
      printf("MESSAGE GENERATOR: ERROR FAILED TO READ TEMPERATURE\n");
      circular_buf_unlock( cbuf);   // unlock buffer
      continue;
    }

    if( fgets( tempBuf, 6, temperatureFile) == NULL){
      printf("MESSAGE GENERATOR: ERROR FAILED TO READ TEMPERATURE(2)\n");
      circular_buf_unlock( cbuf);   // unlock buffer
      continue;
    }

    T = atof(tempBuf);
    T /= 1000;

    /* Option 1: Random AEM in interval [8000, 9000] */
    //toAEM = (uint32_t)((rand() %(9000 - 8000) + 1) + 8000);

    /* Option 2: Random AEM READ from file */
    toAEM = (uint32_t)AEMLIST[rand()%(N_AEM)];

    gettimeofday( &timestamp, NULL);
    time_u = (uint64_t)timestamp.tv_sec;

    memset( buffer, 0, 277*sizeof(char));
    sprintf( buffer, "%"PRIu32"_%"PRIu32"_%"PRIu64"_CPU temperature is : %lf", myAEM, toAEM, time_u, T);
    printf("MESSAGE GENERATOR: %s\n", buffer);



    circular_buf_put( cbuf, time_u, buffer);
    circular_buf_unlock(cbuf);

    pclose (temperatureFile);

    // r = (random % ( upper - lower + 1)) + lower.
    r = (rand() %(((5 * 60) - (1 * 60)) + 1)) + 60;      // Returns a pseudo-random integer between 0 and RAND_MAX
    sleep(r);
  }
}


void *helperMessageGeneratorRoutine(void *cbuf){
  messageGenerator((cbuf_handle_t)cbuf);
}
