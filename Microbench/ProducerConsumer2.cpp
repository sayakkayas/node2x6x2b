#define _GNU_SOURCE
#include <iostream>
#include <cstdlib>
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include<sys/syscall.h>
//#include <stdatomic.h>
#include <errno.h>
#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define mb() asm volatile("mfence":::"memory")
//#define mb()        alternative("lock; addl $0,0(%%esp)", "mfence", X86_FEATURE_XMM2)

#define BUFF_MAX 500
#define ITER 100000
           
using namespace std;

/*size of int is 4 bytes, so each row has 16 integers=16*4=64 Bytes size of cache line at L1,
 *  so buff[BUFF_MAX][16] should occupy BUFF_MAX number of cache line as each row occupies a cache line*/
//Producer writes into buffer,consumer reads from it

//int buff[BUFF_MAX][16];
/*int buff2[BUFF_MAX][16];
int buff3[BUFF_MAX][16];
int buff4[BUFF_MAX][16];
int buff5[BUFF_MAX][16];
int buff6[BUFF_MAX][16];
int buff7[BUFF_MAX][16];
int buff8[BUFF_MAX][16];
int buff9[BUFF_MAX][16];
int buff10[BUFF_MAX][16];
*/
//in and out maybe on the same or different cache lines
//producer reads and writes 'in',consumer just reads it
 int in[16] ;
//in[0]=0;
 //int in=0;
//consumer reads and writes 'out',Producer just reads it
int out[16];
//  out[0]=0;
//int out=0;

pthread_t threads[2];

int start=0;

 void pin_this_thread_to_core(int core_id,pid_t tid) 
 {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
 
  if (core_id < 0 || core_id >= num_cores)
    {  printf("Error in CoreID\n");
        exit(0);

     }  //return EINVAL;

   cpu_set_t cpuset[4];
   CPU_ZERO(&cpuset[core_id]);
   CPU_SET(core_id, &cpuset[core_id]);

   pthread_t current_thread = pthread_self();    
   int no_err= pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset[core_id]);
   if(no_err==0)//success
    {
        if(CPU_ISSET(0,&cpuset[0]))
       printf("Producer Thread %u assigned to core 0\n",tid);

       if(CPU_ISSET(1,&cpuset[1]))
       printf("Consumer Thread %u assigned to core 1\n",tid);

     }
     else
      {
        printf("Failed to pin thread %u on core %d\n",tid,core_id);

       }

}



void* Producer(void*)
{

  //so that both the threads begin together

  
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

          //load in and out
      while ((in[0] + 1) % BUFF_MAX == out[0]); //do nothing
 
       //store buffer
       
        int w=in[0]; 
        // int w1=in[0]; 
    //  buff[in[0]][0] = item;
      /*  buff2[in[0]][0]=item;
       buff3[in[0]][0]=item;
        buff4[in[0]][0]=item;
        buff5[in[0]][0]=item;
       buff6[in[0]][0]=item;
          buff7[in[0]][0]=item;
         buff8[in[0]][0]=item;
         buff9[in[0]][0]=item;
        buff10[in[0]][0]=item;
         */
  
     // printf("Produced item:%d \n",item);
          
          //memory fence here        
             // mb(); //compiler level memory barrier forcing compiler not to reorder memory acceses across barrier
     


            //store in               
           in[0] = (in[0] + 1) % BUFF_MAX;
         
            count++;
   }


}


void* Consumer(void*)
{

 //so that both threads start together

  #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

     pin_this_thread_to_core(1,tid);
 

  int count=0;
     

 
  while (count<ITER)
   {
           //load in and out
   	while (in[0]== out[0]);

                 //load out
                 int w=out[0];
               // int w2=out[0];
             //   int w=buff[out[0]][0];
           /*      w=buff2[out[0]][0];
                  w=buff3[out[0]][0];
                        w=buff4[out[0]][0];
                        w=buff5[out[0]][0];
                       w=buff6[out[0]][0];
                         w=buff7[out[0]][0];
                        w=buff8[out[0]][0];
                        w=buff9[out[0]][0];
                        w=buff10[out[0]][0];
             */          
          // printf("Consumed item %d\n",w);
                    
         //store out
        out[0] = (out[0] + 1) % BUFF_MAX;
      
      count++;
   }

}


int main()
{

// printf("%d\n",sizeof(int));

int i,err;

in[0]=0;
out[0]=0;
  

   for (i = 0; i < 2; i++)
     {

         if(i==0)
         err = pthread_create(&(threads[i]), NULL, &Producer,NULL); 
         else
          err = pthread_create(&(threads[i]), NULL, &Consumer,NULL);

        if (err != 0)
            printf("\n Can't create thread :[%s]\n", strerror(err));
        else
            printf("\n Thread %d created successfully with thread ID %u.\n", i,threads[i]);
    }

 //sleep(1);
 //start=1;
 
    for (i = 0; i < 2; i++)
        pthread_join(threads[i], NULL);



	return 0;
}


