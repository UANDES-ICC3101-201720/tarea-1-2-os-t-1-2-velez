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




void swap(UINT* A, UINT* Z)
{
	UINT aux=*A;
	*A=*Z;
	*Z=aux;
}

//INNECESARIO
int quicksort(UINT* A, int lo, int hi) {
    return 0;
}
// TODO: implement Parallel
struct internalUse
{
    UINT* A;
    int L;
    int R;
};


//partition function
int partition(UINT* A, int L, int R, int pivot)
{
	
    int InitialPivot = A[pivot];
    swap(&A[pivot],&A[R]);

    int aux = L;
    for (int i=L ; i<R ; i++)
    {
        if(A[i]<= InitialPivot)
        {
            swap(&A[i],&A[aux]);
            aux++;
        }
    }
    swap(&A[aux],&A[R]);
    return aux;
}
//Declaring functions for forward use
void parallel_quicksort(UINT* A, int L, int R);

void* q_thread(void* TT)
{
	struct internalUse *Initial= TT;
	parallel_quicksort(Initial->A,Initial->L,Initial->R);
	return NULL;

}    

void parallel_quicksort(UINT* A, int L, int R) 
{
    
	if (L<R)
	{
		int pivot =L+(R-L)/2;
		pivot= partition(A,L,R,pivot);
		struct internalUse *targs =malloc(sizeof(struct internalUse));
		targs->A = A;
		targs->L = L;
		targs->R = pivot-1;
		//threading process (unfixed 25/09)
		pthread_t threads;
		pthread_create(&threads,NULL,q_thread,targs);
		parallel_quicksort(A,pivot+1,R);
		pthread_join(threads,NULL);
		free(targs);
	}
}
// Internal endmark
int main(int argc, char** argv)
{
	printf("[quicksort] Starting up...\n");

	printf("[quicksort] Number of cores available: '%ld'\n",
	   sysconf(_SC_NPROCESSORS_ONLN));

	char* T;
	int E = 0;
	int t = 0;
	int c;

	while((c = getopt (argc, argv, "E:T:")) != -1)
	{
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


	if ((E<0)|(t<3)|(t>9))
	{
	    	printf("values of E and/or T out of range");
	    	exit(1);
    	}
	
	char texte[1];
	sprintf(texte,"%d",E);
	for (int j=0;j<E;++j)
	{
		printf("\n");
		pid_t datagenid = fork();
		if (datagenid==0)
		{
			if (execv("./datagen", argv) < 0)
			    {
				exit(0);
			    }
		}
		else if(datagenid<0)
		{
		    exit(0);
		}
		//socket

		struct sockaddr_un addr;
		int fd;

		if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		{
		    perror("[quicksort] Socket error.\n");
		    exit(-1);
		}

		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, DSOCKET_PATH, sizeof(addr.sun_path) - 1);

		if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		{
		    perror("[quicksort] connect error.\n");
		    close(fd);
		    exit(-1);
		}

		UINT readvalues = 0;
		size_t numvalues = pow(10,E);
		size_t readbytes = 0;


		UINT *readbuf = malloc(sizeof(UINT) * numvalues);

		char begin[20] = "BEGIN U ";
		strcat(begin,texte);




		int rc = strlen(begin);

		if (write(fd, begin, strlen(begin)) != rc)
		{
		    if (rc > 0)
			fprintf(stderr, "[quicksort] partial write.\n");
		    else
		    {
			perror("[quicksort] write error.\n");
			exit(-1);
		    }
		}

		char respbuf[10];
		read(fd, respbuf, strlen(DATAGEN_OK_RESPONSE));
		respbuf[strlen(DATAGEN_OK_RESPONSE)] = '\0';

		if (strcmp(respbuf, DATAGEN_OK_RESPONSE))
		{
		    perror("[quicksort] Response from datagen failed.\n");
		    close(fd);
		    exit(-1);
		}

		while (readvalues < numvalues)
		{
		    readbytes = read(fd, readbuf + readvalues, sizeof(UINT) * 1000);
		    readvalues += readbytes / 4;
		}
		printf("E%i: ", j + 1);
		for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++)
		{
		    printf("%u ", *pv);
		}
		parallel_quicksort(readbuf, 0, numvalues - 1);

		printf("\nS%i: ", j + 1);
		for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++)
		{
		    printf("%u ", *pv);
		}
		free(readbuf);
		rc = strlen(DATAGEN_END_CMD);
		if (write(fd, DATAGEN_END_CMD, strlen(DATAGEN_END_CMD)) != rc)
		{
		    if (rc > 0)
			fprintf(stderr, "[quicksort] partial write.\n");
		    else
		    {
			perror("[quicksort] write error.\n");
			close(fd);
			exit(-1);
		    }
		}
		close(fd);
	}

    exit(0);
}
