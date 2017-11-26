/*
Producer:

while (true) {
  //produce item v 
  while ((in + 1) % n == out)  //do nothing ;
  b [in] = v;
  in = (in + 1) % n;
}

//Consumer:

while (true) {
  while (in == out) ;
  w = b [out];
  out = (out + 1) % n;
 // consume item w 
}*/

//Compile-gcc ProducerConsumerCircular.c -lpthread -o PCB_C -w
//Run-./PCB_C

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include<sys/syscall.h>
//#include <stdatomic.h>
#include <errno.h>
#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define mb() asm volatile("mfence":::"memory")
//#define mb()        alternative("lock; addl $0,0(%%esp)", "mfence", X86_FEATURE_XMM2)
#define BUFF_MAX 1000
#define ITER 50000

/*size of int is 4 bytes, so each row has 16 integers=16*4=64 Bytes size of cache line at L1,
 *  so buff[BUFF_MAX][16] should occupy BUFF_MAX number of cache line as each row occupies a cache line*/
//Producer writes into buffer,consumer reads from it
int buff[BUFF_MAX][16];

//producer reads and writes 'in',consumer just reads it
 int in=0;
//consumer reads and writes 'out',Producer just reads it
  int out=0;


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



void Producer()
{
  
   #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

     pin_this_thread_to_core(0,tid);
  int count=0;

 

  while (count<ITER)
   {
       //produce item  
   	   int item=rand()%100;

      while ((in + 1) % BUFF_MAX == out); //do nothing
 

       buff[in][0] = item;
        // printf("Produced item:%d \n",item);
          
          //memory fence here        
              mb(); //compiler level memory barrier forcing compiler not to reorder memory acceses across barrier
     // in = (in + 1) % BUFF_MAX;
         __sync_fetch_and_add( &in, 1 );
         in=in%BUFF_MAX;
    
            count++;
   }

}


void Consumer()
{


  #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

     pin_this_thread_to_core(1,tid);
  
  int count=0;
 
  while (count<ITER)
   {

   	while (in == out);

 	 int w=buff[out][0];
          // printf("Consumed item %d\n",w);
      //   out = (out + 1) % BUFF_MAX;
      __sync_fetch_and_add( &out, 1 );
       out=out%BUFF_MAX;
     

     count++;
   }

}


int main()
{

// printf("%d\n",sizeof(int));

int i,err;
pthread_t threads[2];

 for (i = 0; i < 2; i++)
     {  if(i==0)
        err = pthread_create(&(threads[i]), NULL, &Producer, NULL);
        else
         err = pthread_create(&(threads[i]), NULL, &Consumer, NULL);
       
        if (err != 0)
            printf("\n Can't create thread :[%s]\n", strerror(err));
        else
            printf("\n Thread %d created successfully with pthread_t POSIX thread ID(not same as kernel TID)  %u.\n", i,threads[i]);
    }

   
    for (i = 0; i < 2; i++)
        pthread_join(threads[i], NULL);



	return 0;
}



/* example of single producer and multiple consumers

   This uses a ring-buffer and no other means of mutual exclusion.
   It works because the shared variables "in" and "out" each
   have only a single writer.  It is an excellent technique for
   those situations where it is adequate.

   If we want to go beyond this, e.g., to handle multiple producers
   or multiple consumers, we need to use some more powerful means
   of mutual exclusion and synchronization, such as mutexes and
   condition variables.
   
 */

/*

#define _XOPEN_SOURCE 500
#define _REENTRANT
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <values.h>
#include <errno.h>
#include <sched.h>

#define BUF_SIZE 10
int b[BUF_SIZE];
int in = 0, out = 0;

pthread_t consumer;
pthread_t producer;

void * consumer_body (void *arg) {
// create one unit of data and put it in the buffer
   //Assumes arg points to an element of the array id_number,
   //identifying the current thread. 
  int tmp;
  int self = *((int *) arg);

  fprintf(stderr, "consumer thread starts\n"); 
  for (;;) {
     // wait for buffer to fill 
     while (out == in);
     // take one unit of data from the buffer 
     tmp = b[out];
     out = (out + 1) % BUF_SIZE;     
     // copy the data to stdout 
     fprintf (stdout, "thread %d: %d\n", self, tmp);
  }
  fprintf(stderr, "consumer thread exits\n"); 
  return NULL;
}

void * producer_body (void * arg) {
//takes one unit of data from the buffer 
   int i;
   fprintf(stderr, "producer thread starts\n"); 
   for (i = 0; i < MAXINT; i++) {
     // wait for space in buffer 
     while (((in + 1) % BUF_SIZE) == out);
     // put value i into the buffer 
     b[in] = i;
     in = (in + 1) % BUF_SIZE;     
  }
  return NULL;
}

int main () {
   int result;
   pthread_attr_t attrs;

   // use default attributes 
   pthread_attr_init (&attrs);

   //create producer thread 
   if ((result = pthread_create (
          &producer, //place to store the id of new thread 
          &attrs,
          producer_body,
          NULL))) {
      fprintf (stderr, "pthread_create: %d\n", result);
      exit (-1);
   } 
   fprintf(stderr, "producer thread created\n"); 

   // create consumer thread 
   if ((result = pthread_create (
      &consumer,
      &attrs,
      consumer_body,
      NULL))) {
     fprintf (stderr, "pthread_create: %d\n", result);
     exit (-1);
   } 
   fprintf(stderr, "consumer thread created\n");

   //give the other threads a chance to run 
   sleep (10);
   return 0;
}
*/

