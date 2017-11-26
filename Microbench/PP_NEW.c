/*Sample program that ping pongs a counter variable among two threads susch that every alternate count is incremented by a thread.The desired experiment is intended for the threads to run in different sockets in order to verify the cross socket  traffic using perf */
//Created by Sayak Chakraborti July 2017
#define _GNU_SOURCE
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
#define MAX 20000000
#define COUNT_NO 5
long long  counter[COUNT_NO];


pthread_mutex_t lock;

pthread_t threads[2];
int start=0;


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


void* doSomeThing()
{

    while(start==0);

 
    if(pthread_equal(threads[0],pthread_self()))
   {
        //int pid=system(SYS_getpid);      
     
    #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    printf("Thread 0 TID %d\n",tid); 
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif
      //int tid=system(SYS_gettid);
         //printf("PID %d Thread 0 TID %d\n",pid,tid);
     pin_this_thread_to_core(0,tid);
   printf("Thread 0 counter address %p\n",(void*)&counter[0]);

   }
   if(pthread_equal(threads[1],pthread_self()))
    {
       #ifdef SYS_gettid
       pid_t tid = syscall(SYS_gettid);
       printf("Thread 1 TID %d\n",tid); 
      #else
      #error "SYS_gettid unavailable on this system"  
       #endif
     pin_this_thread_to_core(1,tid);
     printf("Thread 1 counter address %p\n",(void*)&counter[0]);


    }


   
    pthread_t id = pthread_self();

    if(pthread_equal(id,threads[0]))
    {  int k;
       
         while(counter[0]<MAX)
         {   
           // printf("\n First thread processing...\n");
              //printf("%ld \n",counter[0]);       

            if(counter[0]%2==0)
             { //atomic_fetch_add((args->counter_ptr),1);
              
                pthread_mutex_lock(&lock);
                    //printf("Thread 1\n");
                     for(k=0;k<COUNT_NO;k++)
                  counter[k]=counter[k]+1;       

                 
                    //   printf("Thread 1  counter[0] %ld\n",counter[0]);   

               pthread_mutex_unlock(&lock);
             }
         }
    }
    else
       { 

          int k;

         while(counter[0]<MAX)
         {
           //printf("\n Second thread processing...\n");
           
           if(counter[0]%2==1)
            {  //atomic_fetch_add((args->counter_ptr),1);
               pthread_mutex_lock(&lock);
                  // printf("Thread 2\n");
                for(k=0;k<COUNT_NO;k++)
                   counter[k]=counter[k]+1;
                  // printf("Thread 2  counter[0] %ld\n",counter[0]);

               pthread_mutex_unlock(&lock);
               
               
             } 
          }

        }
    //usleep(5);
    return NULL;
}

//long int counter[20];

int main(void)
{


    int i = 0;
    int err;
   // long int counter[10];
  for(i=0;i<10;i++)  
    counter[i]=0;
      

  printf("Main thread Counter address %p\n",(void*)&(counter[0]));
  //printf("MAX address %p\n",(void*)&MAX);
//  printf("args address %p\n",(void*)&args);

  // args[0].counter_ptr=&counter[0];
   //args[1].counter_ptr=&counter[0];
 



    for (i = 0; i < 2; i++)
     {
        err = pthread_create(&(threads[i]), NULL, &doSomeThing, NULL);
        if (err != 0)
            printf("\n Can't create thread :[%s]\n", strerror(err));
        else
            printf("\n Thread %d created successfully with pthread_t POSIX thread ID(not same as kernel TID)  %u.\n", i,threads[i]);
    }

     start=1;

    for (i = 0; i < 2; i++)
        pthread_join(threads[i], NULL);
 
    printf("Sizeof long int %d\n",sizeof(long int));

     printf("Counter value at end %ld\n",counter[0]);
     printf("Coherence events 3 times %ld\n",3*counter[0]);
    
return 0; 
}
