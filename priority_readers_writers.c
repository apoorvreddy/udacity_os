#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int shared_x = 0; // shared global variable
int resource_counter = 0; // proxy variable for entering the critical section
int waiting_readers = 0; // used to determine priority of readers over writers

int NUM_READS = 5; // num times each read thread is invoked
int NUM_WRITES = 5; // num times each write thread is invoked
int NUM_READERS = 10; // number of reader threads
int NUM_WRITERS = 3; // number of writer threads


pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // mutex lock for shared variable
pthread_cond_t c_read = PTHREAD_COND_INITIALIZER; // reader waits on this condition variable
pthread_cond_t c_write = PTHREAD_COND_INITIALIZER; // writer waits on this condition variable

void *reader(void *param);
void *writer(void *param);

int main(int argc, char* argv[]){
	pthread_t rt[NUM_READERS];
	pthread_t wt[NUM_WRITERS];
	
	int i=0;
	int reader_thread_num[NUM_READERS];
	int writer_thread_num[NUM_WRITERS];

	/* create reader threads */
	for(i=0; i<NUM_READERS; i++){
		reader_thread_num[i] = i;
 		if(pthread_create(&rt[i], NULL, reader, (void*)&reader_thread_num[i]) != 0) {
			fprintf(stderr, "Unable to create reader thread\n");
			exit(1);
		}
	}

	/* create writer threads */
	for(i=0; i<NUM_WRITERS; i++){
		writer_thread_num[i] = i;
		if(pthread_create(&wt[i], NULL, writer, (void*)&writer_thread_num[i]) != 0) {
			fprintf(stderr, "Unable to create writer thread\n");
			exit(1);
		}
	}

	/* wait for created reader threads to exit */
	for(i=0; i<NUM_READERS; i++){
		pthread_join(rt[i], NULL);
	}

	/* wait for created writer threads to exit */
	for(i=0; i<NUM_WRITERS; i++){
		pthread_join(wt[i], NULL);
	}

	printf("Parent process quiting\n");	
	return 0;
}

void *writer(void *param){
	int *i = (int *)param;
	int num = 0;

	for(num=0; num<NUM_READS; num++){
		// Wait so that reads and writes do not all happen at once
		usleep(1000 * (random() % (NUM_READERS + NUM_WRITERS)));

		/* ENTER CRITICAL SECTION */
		pthread_mutex_lock(&m);
		while(resource_counter != 0)
			pthread_cond_wait(&c_write, &m);

		resource_counter = -1;
		pthread_mutex_unlock(&m);

		/* CRITICAL SECTION */
		printf("Writer[%d] writes value %d\n", *i, ++shared_x); fflush(stdout);

		/* EXIT CRITICAL SECTION */
		pthread_mutex_lock(&m);
		resource_counter = 0;
		if(waiting_readers > 0)
			pthread_cond_broadcast(&c_read);
		else
			pthread_cond_signal(&c_write);
		pthread_mutex_unlock(&m);
	}
	pthread_exit(0);
}

void *reader(void *param){
	int *i = (int*)param;
	int num = 0;

	for(num=0; num<NUM_WRITES; num++){
		// Wait so that reads and writes do not all happen at once
		usleep(1000 * (random() % (NUM_READERS + NUM_WRITERS)));

		/* ENTER CRITICAL SECTION */
		pthread_mutex_lock(&m);
		waiting_readers ++;
		while(resource_counter == -1)
			pthread_cond_wait(&c_read, &m);
		resource_counter++;
		waiting_readers--;
		pthread_mutex_unlock(&m);

		/* CRITICAL SECTION */
		printf ("Reader[%d] reads value %d\n", *i, shared_x);  fflush(stdout);
		
		/* EXIT CRITICAL SECTION */
		pthread_mutex_lock(&m);
		resource_counter--;
		if(resource_counter == 0)
			pthread_cond_signal(&c_write);

		pthread_mutex_unlock(&m);
	}
	pthread_exit(0);
}