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
//#define MAX 60000000
long int MAX=60000000;

pthread_t threads[2];
int start=0;
long int spin_thread_1=0;
long int spin_thread_2=0;
long int success_thread1=0;
long int success_thread2=0;
int flag_thread1=0;
int flag_thread2=0;

 struct arg_struct
{
   pthread_t tid;
  //atomic_int* counter_ptr;
    long int* counter_ptr;
 long int* counter2_ptr;
    //long int* counter3_ptr;
    //long int* counter4_ptr;
    //long int* counter5_ptr;
};

struct arg_struct  args[2];

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


void* doSomeThing(void *arguments)
{

    while(start==0);

 
 struct arg_struct* args=arguments;


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
   printf("Thread 0 counter address %p\n",args->counter_ptr);

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
     printf("Thread 1 counter address %p\n",args->counter_ptr);


    }


   // struct arg_struct* arg=arguments;
    //unsigned long i = 0;
    pthread_t id = pthread_self();

    if(pthread_equal(id,threads[0]))
    { 
       printf("Thread 0 args:%p args->counter_ptr:%p  success_threads1 address:%p    flag_thread1:%p spin_thread1:%p\n",(void*)&args,(void*)&args->counter_ptr,(void*)&success_thread1,(void*)&flag_thread1,(void*)&spin_thread_1); 
      
         while(*(args->counter_ptr)<MAX)
         {   
           // printf("\n First thread processing...\n");
       
            if(*(args->counter_ptr)%2==0)
             { //atomic_fetch_add((args->counter_ptr),1);
               *(args->counter_ptr)=*(args->counter_ptr)+1;
                 success_thread1++;
                flag_thread1=1;
                }
             else
             {
               if(flag_thread1==1)
                { spin_thread_1++;
                  flag_thread1=0;
                }
             }
         }
    }
    else
       { 

          printf("Thread 1  args:%p args->counter_ptr:%p success_threads2 address:%p    flag_thread2:%p spin_thread2:%p\n",(void*)&args,(void*)&args->counter_ptr,(void*)&success_thread2,(void*)&flag_thread2,(void*)&spin_thread_2);


         while(*(args->counter_ptr)<MAX)
         {
           //printf("\n Second thread processing...\n");
           
           if(*(args->counter_ptr)%2==1)
            {  //atomic_fetch_add((args->counter_ptr),1);
               *(args->counter_ptr)=*(args->counter_ptr)+1;
               
               success_thread2++;
               flag_thread2=1;
               }
             else
             {
                 if(flag_thread2==1) 
                  { spin_thread_2++;
                     flag_thread2=0;
                  }
             } 
          }

        }
    //usleep(5);
    return NULL;
}

int main(void)
{

//pid_t pid;
//pid=fork();
 
//printf("Main thread Counter address %p\n",(void*)&counter);
 //if(pid==0)
 {
 //  printf("Child\n");
    buf();

 }
 //else
 //printf("Parent\n");
return 0;
}

void buf()
{
    int i = 0;
    int err;


    //atomic_int counter=0;
      // long int counter=0;
     //  long int counter2=0;
      // long int counter3=0;
      // long int counter4=0;
      // long int counter5=0;
      long int counter=0;
  printf("Main thread Counter address %p\n",(void*)&counter);
  printf("MAX address %p\n",(void*)&MAX);
  printf("args address %p\n",(void*)&args);

   args[0].counter_ptr=&counter;
   args[1].counter_ptr=&counter;
 



    for (i = 0; i < 2; i++)
     {
        err = pthread_create(&(threads[i]), NULL, &doSomeThing, (void*)&args[i]);
        if (err != 0)
            printf("\n Can't create thread :[%s]\n", strerror(err));
        else
            printf("\n Thread %d created successfully with pthread_t POSIX thread ID(not same as kernel TID)  %u.\n", i,threads[i]);
    }

     start=1;

    for (i = 0; i < 2; i++)
        pthread_join(threads[i], NULL);

     printf("Counter value at end %ld\n",counter);
    printf("Successful increments Thread1 %ld Thread2 %ld\n",success_thread1,success_thread2);
    printf("Unsuccessful spins Thread1 %ld Thread2 %ld\n",spin_thread_1,spin_thread_2);
 
}
