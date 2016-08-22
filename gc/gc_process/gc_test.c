#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <time.h>
#include "gc.h"

struct node_t{
	int val;
};

pthread_t t_id[100]={0};
void *ptr;
int n_thread,t_now=0,n_op_exp;
int n_op=0;

void* run_thread(void *arg){
	int pos_add=1, pos_read;
	pthread_t t_no=pthread_self();
	void *local_ptr=NULL;
	
	// Loop till no of operation is reached
	while(n_op<n_op_exp){	
		if(pthread_equal(t_no,t_id[(t_now%n_thread)])){
			// Allocate new memory region to shared ptr
			gcalloc(&ptr,sizeof(struct node_t));
			n_op++; 

			printf("[pid %d addn %d]: %d\n",(int)syscall(__NR_gettid),n_op,pos_add);			
			((struct node_t*)ptr)->val=pos_add++;

			sleep(2);
			t_now++;	// Next thread to allocate ptr in its next loop 
		}
		else{
			// Read memory region referenced by ptr
			gcassign(&local_ptr,ptr);

			if(local_ptr){
				pos_read=((struct node_t*)local_ptr)->val;
				n_op++;
				printf("[pid %d read %d]: %d\n",(int)syscall(__NR_gettid),n_op,pos_read);
			}

			sleep(1);
		}
	}
}

int main(int argc,char **argv){	
	time_t start_t,end_t;
	int i;
	gc_reg();

	if(argc!=3){
		printf("Usage: <program> <no of threads> <no of operations>\n");
		return 0;
	}
	
	n_thread=atoi(argv[1]);		// number of threads
	n_op_exp=atoi(argv[2]);		// number of operation

	printf("[process tid]: %d %d %d\n", getpid(),n_op_exp,n_thread);
	sleep(5);
	
	time(&start_t);				// start clock

	// Create Threads
	for(i=0;i<n_thread;i++)
		pthread_create(&t_id[i],NULL,&run_thread,NULL);
	
	sleep(2);	// Allow threads to start

	for(i=0;i<n_thread;i++)
		pthread_join(t_id[i],NULL);
	
	time(&end_t);				// end clock

	// check outut for calculating throughput
	printf("[process time]: %0.2f\n",difftime(end_t,start_t));
	return 0;
}	
	
