#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "types.h"
#include "const.h"
#include "util.h"

void swap(UINT* A, int a, int z){
	int temp = A[a];
	A[a] = A[z];
	A[z] = temp; 
}

// TODO: implement
int quicksort(UINT* A, int lo, int hi) {
    return 0;
}
// TODO: implement
struct internalUse
{
    UINT* A;
    int lo;
    int hi;
    int d;
};



int partition(UINT* A, int lo, int hi, int pi) {
	
    int pVal = A[pi];
    swap(A, pi, hi);
    int aux = lo;
    for (int i=lo ; i<hi ; i++)
    {
        if(A[i]<= pVal)
        {
            swap(A, i, aux);
            aux++;
        }
    }
    swap(A, aux, hi);
    return aux;
}

void parallel_quicksort(UINT* A, int lo, int hi, int d);

void* q_thread(void *init)
{
    struct internalUse *start = init;
    parallel_quicksort(start -> A, start -> lo, start -> hi, start -> d);
    return NULL;
}    

void parallel_quicksort(UINT* A, int lo, int hi, int d) {
    
    if (lo < hi)
    {
        int pi = lo + (hi - lo)/2;
        pi= partition(A, lo, hi, pi);
        if (d> 0)
        {
	    struct internalUse arg = {A, lo, pi-1, d};
            pthread_t thread;
            int ret = pthread_create(&thread, NULL, q_thread, &arg);
            if(ret == 0)
		{
 			printf("Thread creating error\n");
		}
            parallel_quicksort(A, pi+1, hi, d);
            pthread_join(thread, NULL);
        }
        else
        {
            quicksort(A, lo, pi-1);
            quicksort(A, pi+1, hi);
        }
     }
}
// Internal endmark
int main(int argc, char** argv) {
    printf("[quicksort] Starting up...\n");

    /* Get the number of CPU cores available */
    printf("[quicksort] Number of cores available: '%ld'\n",
           sysconf(_SC_NPROCESSORS_ONLN));

    /* TODO: parse arguments with getopt */
    char* T;
    int E = 0;
    int t = 0;
    int c;

    while((c = getopt (argc, argv, "E:T:")) != -1){
    	switch (c)
    	{
    		case 'E':
			E = atoi(optarg);
			printf("E: %d\n",E);
			break;
		case 'T':
			T = optarg;
			printf("T: %s\n",T);
			break;
    	}

    }
	t = atoi(T);


    if (E<0){
    	printf("E value out of range");
    	exit(0);
    	}

    int size = 10;

    for(int i=1;i<=t;i++){
    	size = size*10;
}

    /* TODO: start datagen here as a child process. */
pid_t datagen_id = fork();
    char *datagen_file[]={"./datagen",NULL};


    if(datagen_id == 0)
    {
    	printf("%s%d\n","PID for Datagen : ", getpid());
    	execvp(datagen_file[0],datagen_file);

    }
    else if(datagen_id == -1)
    {
      printf("Datagen Error \n");
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
    for (int i = 0; i < E; i++) {
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
