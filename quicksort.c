#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <assert.h>
#include "types.h"
#include "const.h"
#include "util.h"

// TODO: implement
// Macro para swap.
#define SWAP(x,y) do {\
    __typeof__(x) tmp = x;\
    x = y;\
    y = tmp;\
} while(0)

int partition(UINT *array, int lo, int hi)
{
    int pivotValue = array[hi];
    int storeIndex = lo;
    for (int i=lo ; i<hi ; i++)
    {
        if (array[i] <= pivotValue)
        {
            SWAP(array[i], array[storeIndex]);
            storeIndex++;
        }
    }
    SWAP(array[storeIndex], array[hi]);
    return storeIndex;
}

 void quicksort(UINT *array, int lo, int hi)
 {
      if (hi > lo)
      {
         int pivotIndex = lo + (hi - lo)/2;
         pivotIndex = partition(array, lo, hi);
         quicksort(array, lo, pivotIndex-1);
         quicksort(array, pivotIndex+1, hi);
      }
 }

//----------------------------------------------------------

// TODO: implement quicksort parallel

struct quicksort_variables
{
    UINT *array;
    int lo;
    int hi;
};

void parallel_quicksort(UINT *array, int lo, int hi);

void* quicksort_thread(void *init)
{
    struct quicksort_variables *start = init;
    parallel_quicksort(start->array, start->lo, start->hi);
    return NULL;
}

void parallel_quicksort(UINT *array, int lo, int hi)
{
    if (hi > lo)
    {
        int pivotIndex = lo + (hi - lo)/2;
        pivotIndex = partition(array, lo, hi);
        int sub_process = 2; //cantidad arbitraria p de threads, 2 para la maquina virtual default
        if (sub_process-- > 0)
        {
            struct quicksort_variables vars = {array, lo, pivotIndex-1};
            pthread_t new_thread;
            int ret = pthread_create(&new_thread, NULL, quicksort_thread, &vars);
            assert((ret == 0) && "Thread creation failed");

            parallel_quicksort(array, pivotIndex+1, hi);

            pthread_join(new_thread, NULL);
        }
        else
        {
            quicksort(array, lo, pivotIndex-1);
            quicksort(array, pivotIndex+1, hi);
        }
    }
}
//-------------------------------------------------------

int main(int argc, char** argv) {
    printf("[quicksort] Starting up...\n");

    /* Get the number of CPU cores available */
    printf("[quicksort] Number of cores available: '%ld'\n",
           sysconf(_SC_NPROCESSORS_ONLN));

    /* parse arguments with getopt */
    int E = 0;
    char* T;
    int t_num = 0;
    int c;

    while((c = getopt (argc, argv, "E:T:")) != -1){
    	switch (c)
    	{
    		case 'E':
						E = atoi(optarg);
						break;
				case 'T':
						T = optarg;
						printf("T: %s\n",T);
						break;
    	}

    }

    t_num = atoi(T);


    if (E<0 || (t_num<3 || 9<t_num)){
    	printf("Program terminated, value(s) out of range");
    	exit(0);
    }

    int size = 10;

    for(int i=1;i<=t_num;i++){
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
    for (int i = 0; i < 2; i++){
        /* T value 3 hardcoded just for testing. */
        char begin[10] = "BEGIN U ";
        strcat(begin,T);
        printf("begin: %s\n",begin);
        int rc = strlen(begin);
        /* Request the random number stream to datagen */
        if (write(fd, begin, strlen(begin)) != rc) {
            if (rc > 0) fprintf(stderr, "[quicksort] partial write.\n");
            else {
                perror("[quicksort] write error.\n");
                exit(-1);
            }
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
        size_t numvalues = pow(10, t_num);
        size_t readbytes = 0;

        UINT *readbuf = malloc(sizeof(UINT) * numvalues);

        while (readvalues < numvalues) {
            /* read the bytestream */
            readbytes = read(fd, readbuf + readvalues, sizeof(UINT) * 1000);
            readvalues += readbytes / 4;
        }

        /* Print out the values obtained from datagen */
				/* Print out the values obtained from datagen */

        for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++) {
            printf("%u,", *pv);
        }

			//	printf("Hasta aqui funciona\n");
        printf("\n \n");

        quicksort(readbuf,0,numvalues);
        //parallel_quicksort(readbuf,0,numvalues);

        for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++) {
            printf("%u,", *pv);
        }
        printf("\n");


        free(readbuf);


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
