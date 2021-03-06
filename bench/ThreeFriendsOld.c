/*Sample program that ping pongs a counter variable among two threads susch that every alternate count is incremented by a thread->The desired experiment is intended for the threads to run in different sockets in order to verify the cross socket  traffic using perf */
//Created by Sayak Chakraborti July 2017
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
//#include <stdatomic->h>
#include <errno.h>
#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define MAX 20000000
#define COUNT_NO 40

long long  counter1[COUNT_NO];
long long counter2[COUNT_NO];
long long counter3[COUNT_NO];

struct arg
{
  long long * counter_ptr;
  long long * counter_ptr2;
  long long * counter_ptr3;
};

struct arg arguments[3];

pthread_t threads[3];
int start=0;


void pin_this_thread_to_core(int core_id,pid_t tid) 
{
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
 
  if (core_id < 0 || core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset[4];
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

        if(CPU_ISSET(3,&cpuset[3]))
       printf("Thread %u assigned to core 3\n",tid);

     }
     else
      {
        printf("Failed to pin thread %u on core %d\n",tid,core_id);

       }

}


void* doSomeThing(void* arguments)
{

    while(start==0);

    struct arg* args=arguments;

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
   //printf("Thread 0 counter address %p\n",(void*)&counter[0]);

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
     //printf("Thread 1 counter address %p\n",(void*)&counter[0]);


    }

  if(pthread_equal(threads[2],pthread_self()))
    {
       #ifdef SYS_gettid
       pid_t tid = syscall(SYS_gettid);
       printf("Thread 1 TID %d\n",tid); 
      #else
      #error "SYS_gettid unavailable on this system"  
       #endif
     pin_this_thread_to_core(3,tid);
     //printf("Thread 2 counter address %p\n",(void*)&counter[0]);


    }


   
    pthread_t id = pthread_self();

    if(pthread_equal(id,threads[0]))
    {  
       
         while(1)
         {   
             if( *(args->counter_ptr)>=MAX &&  *(args->counter_ptr2)>=MAX  && *(args->counter_ptr3)>=MAX  )
               break;

             if(*(args->counter_ptr) %2==0 && *(args->counter_ptr)< MAX)
             { //atomic_fetch_add((args->counter_ptr),1);
              
                 *(args->counter_ptr)=*(args->counter_ptr)+1;

              }
   


             if(*(args->counter_ptr2)%2==0 && *(args->counter_ptr2)< MAX)
             { //atomic_fetch_add((args->counter_ptr),1);
              
                 *(args->counter_ptr2)=*(args->counter_ptr2)+1;

             }


             if(*(args->counter_ptr3)%2==0 && *(args->counter_ptr3)< MAX)
             { //atomic_fetch_add((args->counter_ptr),1);
              
                 *(args->counter_ptr3)=*(args->counter_ptr3)+1;

            
             }
   
   

         }//while close
    }
   


         if(pthread_equal(id,threads[1]))
         { 

          

         	while(*(args->counter_ptr)<MAX)
         	{
           
           		if(*(args->counter_ptr)%2==1)
            	{ 
               		 *(args->counter_ptr)=*(args->counter_ptr)+1;
                  
                  }
               
             } 
          

        }



    if(pthread_equal(id,threads[2]))
    {  
       
         while(1)
         {   
             if( *(args->counter_ptr2)>=MAX && *(args->counter_ptr3)>=MAX )
               break;

             


             if(*(args->counter_ptr2)%2==1 && *(args->counter_ptr2)< MAX)
             { //atomic_fetch_add((args->counter_ptr),1);
              
                 *(args->counter_ptr2)=*(args->counter_ptr2)+1;

               
             }


             if(*(args->counter_ptr3)%2==1 && *(args->counter_ptr3)< MAX)
             { //atomic_fetch_add((args->counter_ptr),1);
              
                 *(args->counter_ptr3)=*(args->counter_ptr3)+1;

               }
   
   

         }//while close
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
  counter1[0]=0;
  counter2[0]=0;
  counter3[0]=0;
 
  printf("Main thread Counter1[0] address %p\n",(void*)&(counter1[0]));
  printf("Main thread Counter2[0] address %p\n",(void*)&(counter2[0]));
  printf("Main thread Counter3[0] address %p\n",(void*)&(counter3[0]));
 
  arguments[0].counter_ptr=&counter1[0];
  arguments[0].counter_ptr2=&counter2[0];
  arguments[0].counter_ptr3=&counter3[0];
  
  arguments[1].counter_ptr=&counter1[0];
  arguments[1].counter_ptr2=NULL;
  arguments[1].counter_ptr3=NULL;

  arguments[2].counter_ptr=NULL;
  arguments[2].counter_ptr2=&counter2[0];
  arguments[2].counter_ptr3=&counter3[0];
  
  
 


    for (i = 0; i < 3; i++)
     {
        err = pthread_create(&(threads[i]), NULL, &doSomeThing, (void*)&arguments[i]);
        if (err != 0)
            printf("\n Can't create thread :[%s]\n", strerror(err));
        else
            printf("\n Thread %d created successfully with pthread_t POSIX thread ID(not same as kernel TID)  %u\n", i,threads[i]);
    }

     start=1;

    for (i = 0; i < 3; i++)
        pthread_join(threads[i], NULL);
 

      printf("Counter1[0] value at end %ld\n",counter1[0]);
      printf("Counter2[0] value at end %ld\n",counter2[0]);
      printf("Counter3[0] value at end %ld\n",counter3[0]);
    
return 0; 
}
