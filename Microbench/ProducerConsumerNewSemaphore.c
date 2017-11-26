#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include<sys/syscall.h>
//#include <stdatomic.h>
#include <errno.h>
#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define BUFFER_MAX 102400

//shared variables
 int buffer[BUFFER_MAX];
 int in=0;
 int out=0;
 int start=0;
//shared semaphore
sem_t mutex;
sem_t empty;
sem_t full; 



void pin_this_thread_to_core(int core_id,pid_t tid) 
{
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
 
  if (core_id < 0 || core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset[2];
   CPU_ZERO(&cpuset[core_id]);
   CPU_SET(core_id, &cpuset[core_id]);

   pthread_t current_thread = pthread_self();    
   int no_err= pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset[core_id]);
   if(no_err==0)//success
    {
        if(CPU_ISSET(0,&cpuset[0]))
       printf("Thread %u assigned to core 0\n",tid);

       if(CPU_ISSET(1,&cpuset[1]))
       printf("Thread %u assigned to core 1\n",tid);

     }
     else
      {
        printf("Failed to pin thread %u on core %d\n",tid,core_id);

       }

}




void Consumer()
{
 while(start==0);


   #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    printf("Consumer: Thread 1 TID %d\n",tid); 
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif
      //int tid=system(SYS_gettid);
         //printf("PID %d Thread 0 TID %d\n",pid,tid);
     pin_this_thread_to_core(1,tid);
  

 
   long long count=0; 

	do
 	{
             sem_wait(&full);
             sem_wait(&mutex);
             
              //consume an item      
               int item=buffer[out];
                out++;
               //printf("Consumer %d\n",item);
             sem_post(&mutex);
             sem_post(&empty);    
   
      count++;
  	}while(count<=100000);

}


void Producer()
{
  while(start==0);

    #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    printf("Producer : Thread 0 TID %d\n",tid); 
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif
      //int tid=system(SYS_gettid);
         //printf("PID %d Thread 0 TID %d\n",pid,tid);
     pin_this_thread_to_core(0,tid);
  


    long long count=0; 

	do
 	{
             sem_wait(&empty);
             sem_wait(&mutex);
              //produce an item      
             int item=rand()%100;
               buffer[in]=item;
                in++;
              // printf("Producer %d\n",item);
             
             sem_post(&mutex);
             sem_post(&full);    

    count++;
  	}while(count<=100000);

}


int main()
{
	 // int sem_init(sem_t *sem, int pshared, unsigned int value);
      //pshared whether shared between threads of the same process or processes
	 sem_init(&mutex,0,1);
	 sem_init(&empty,0,BUFFER_MAX);
	 sem_init(&full,0,0);

	 pthread_t threads[2];

	 int i,err;
	  for(i=0;i<2;i++)
	  {
	  	 if(i==0)
          err=pthread_create(&threads[i],NULL,&Producer,NULL);
         else
          err=pthread_create(&threads[i],NULL,&Consumer,NULL);
          
           if (err != 0)
            printf("\n Can't create thread :[%s]\n", strerror(err));
           else
            printf("\n Thread %d created successfully with thread ID %u.\n", i,threads[i]);

	  }
	 


   start=1;

    for (i = 0; i < 2; i++)
        pthread_join(threads[i], NULL);


	return 0;
}
