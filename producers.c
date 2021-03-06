#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <prodcon.h>
#include <math.h>

char *service;
char *host = "localhost";
double rate; 
int bad; 

double poissonRandomInterarrivalDelay( double r );
int connectsock(char *host, char *service, char *protocol);
void *worker(void *ign);
int streamFromRandom(int randomData, int socket, int size);
/*
**	Producer Client
*/
int main(int argc, char *argv[])
{
	int pNumber;
	size_t i;
	double sleepTime;
	int seconds; 
	int usec; 

	switch (argc) {
		case 5:
			service = argv[1];
			pNumber = atoi(argv[2]);
			rate = strtof(argv[3], NULL); 
			bad = atoi(argv[4]);
			break;
		case 6:
			host = argv[1];
			service = argv[2];
			pNumber = atoi(argv[3]);
			rate = strtof(argv[4], NULL); 
			bad = atoi(argv[5]);
			break;
		default:
			printf("wrong number of arguments \n");	
			fflush(stdout);
			return -1;
	}
	pNumber %= 2000;
	pthread_t threads[pNumber];

	for (i = 0; i < pNumber; i++)
	{
		sleepTime = poissonRandomInterarrivalDelay(rate);
		seconds = (int) sleepTime; 
		usec = 1000000*(sleepTime - seconds); 
		printf("sleepTime %lf, seconds %i, usec %i\n", sleepTime, seconds, usec);
		fflush(stdout); 
		sleep(seconds);
		usleep(usec);
		pthread_create(&threads[i], NULL, worker, NULL);
	}

	for (i = 0; i < pNumber; i++)
	{
		pthread_join(threads[i], NULL);
	}
	exit(0);
}

void *worker(void *ign)
{
	int csock;
	char* buf =(char *) malloc(BUFSIZE);
	int size;
	int netInt;
	int randomData = open("/dev/urandom", O_RDONLY);

	if ((csock = connectsock(host, service, "tcp")) == 0)
	{
		fprintf(stderr, "Cannot connect to server.\n");
		exit(-1);
	}
	int dice = rand()%100; 
	if( dice <= bad){
		printf("Slow client with dice %i\n", dice); 
		sleep(SLOW_CLIENT); 
	}
	write(csock, "PRODUCE\r\n", 10);
	size = (rand() + 1) % MAX_LETTERS;
	read(csock, buf, BUFSIZE);
	if (strncmp(buf, "GO\r\n", 4) == 0)
	{
		netInt = htonl(size);
		write(csock, &netInt, 4);
		read(csock, buf, BUFSIZE);
			if (strncmp(buf, "GO\r\n", 4) == 0)
			{
					if(streamFromRandom(randomData, csock, size) < 0){
						printf("encountered problem when streamin \n");
						fflush(stdout);
					}else{
						read(csock, buf, BUFSIZE); //should be done
					}
			}
	}
	free(buf);
	close(csock);
	close(randomData);
	pthread_exit(0);
}

int streamFromRandom(int randomData, int socket, int size){
	char * buf = (char *) malloc(BUFSIZE); 
	int cc = 0;
	int readUpTo = 0; 
	while(readUpTo < size){
		cc = read(randomData, buf, BUFSIZE);
		if( cc < 0){
			free(buf);
			return -1; 
		}
		readUpTo += cc; 
		write(socket, buf, cc); 
	}
	free(buf);
	return 0;
}

/*
**      Poisson interarrival times. Adapted from various sources
**      r = desired arrival rate
*/
double poissonRandomInterarrivalDelay( double r )
{
    return (log((double) 1.0 - 
			((double) rand())/((double) RAND_MAX)))/-r;
}

