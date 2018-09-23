#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "const.h"
#include "util.h"

// TODO: implement
int quicksort(UINT* number, int first, int last) {
		int i,j,pivot,temp;
		if(first<last){
			pivot=first;
			i=first;
			j=last;
			
			while(i<j){
				while(number[i]<=number[pivot]&&i<last)
					i++;
				while(number[i]>number[pivot])
					j--;
				if(i<j){
					temp=number[i];
					number = number[j];
					number[j]=temp;
				}
			}
			
			temp=number[pivot];
			number[pivot]=number[j];
			number[j]=temp;
			quicksort(number,first,j-1);
			quicksort(number,j+1,last);
			
		}
		
    return 0;
}

// TODO: implement
int parallel_quicksort(UINT* A, int lo, int hi) {
    return 0;
}

int main(int argc, char** argv) {
    printf("[quicksort] Starting up...\n");

    /* Get the number of CPU cores available */
    printf("[quicksort] Number of cores available: '%ld'\n",
           sysconf(_SC_NPROCESSORS_ONLN));

    /* parse arguments with getopt */
    int E = 0;
    int T = 0;
    
    while((c = getopt (argc, argv, "E:T:")) != -1){
    	switch(c)
    	{
    		case 'E':
    			E = atoi(optarg);
    		case 'T':
    			T = atoi(optarg);
    	}
    }
    
    if (E<0 || T<3 || 9<T){
    	printf("Program terminated, value(s) out of range");
    	exit(0);
    }
    
    size = 10;
    
   for(int i=1;i<=T;i++){
    	size = size*10;
    }

    /* start datagen here as a child process. */
		pid_t datagen_id = fork();
    char *datagen_file[]={"./datagen",NULL};


    if(datagen_id == 0)
    {
    	printf("%s%d\n","PID Fork Datagen : ", getpid());
    	execvp(datagen_file[0],datagen_file);

    }
    else if(datagen_id == -1)
    {
      printf("error al crear fork \n");
    }
    
    /* Create the domain socket to talk to datagen. */
    struct sockaddr_un addr;
    int fd;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("[quicksort] Socket error.\n");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, DSOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("[quicksort] connect error.\n");
        close(fd);
        exit(-1);
    }

    /* DEMO: request two sets of unsorted random numbers to datagen */
    for (int i = 0; i < 2; i++) {
        /* T value 3 hardcoded just for testing. */
        char *begin = "BEGIN U 3";
        int rc = strlen(begin);

        /* Request the random number stream to datagen */
        if (write(fd, begin, strlen(begin)) != rc) {
            if (rc > 0) fprintf(stderr, "[quicksort] partial write.\n");
            else {
                perror("[quicksort] write error.\n");
                exit(-1);
            }
        }

        /* validate the response */
        char respbuf[10];
        read(fd, respbuf, strlen(DATAGEN_OK_RESPONSE));
        respbuf[strlen(DATAGEN_OK_RESPONSE)] = '\0';

        if (strcmp(respbuf, DATAGEN_OK_RESPONSE)) {
            perror("[quicksort] Response from datagen failed.\n");
            close(fd);
            exit(-1);
        }

        UINT readvalues = 0;
        size_t numvalues = pow(10, 3);
        size_t readbytes = 0;

        UINT *readbuf = malloc(sizeof(UINT) * numvalues);

        while (readvalues < numvalues) {
            /* read the bytestream */
            readbytes = read(fd, readbuf + readvalues, sizeof(UINT) * 1000);
            readvalues += readbytes / 4;
        }

        /* Print out the values obtained from datagen */
        for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++) {
            printf("%u\n", *pv);
        }

        free(readbuf);
    }

    /* Issue the END command to datagen */
    int rc = strlen(DATAGEN_END_CMD);
    if (write(fd, DATAGEN_END_CMD, strlen(DATAGEN_END_CMD)) != rc) {
        if (rc > 0) fprintf(stderr, "[quicksort] partial write.\n");
        else {
            perror("[quicksort] write error.\n");
            close(fd);
            exit(-1);
        }
    }

    close(fd);
    exit(0);
}
