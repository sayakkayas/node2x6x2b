/*Compile g++ PPWA.cpp -lpthread -std=c++0x -o PPWAcpp -w*/
#define _GNU_SOURCE
#include <iostream>
#include <cstdlib>
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>
//#include <chrono>
#include<sys/syscall.h>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define MAX 100

using namespace std;


pthread_t threads[2];
std::atomic<int> start;
std::atomic<long int> counter = ATOMIC_VAR_INIT(0);

int flag_thread1=0;
int flag_thread2=0;
long int spin_thread_1=0;
long int spin_thread_2=0;


 struct arg_struct
 {
   pthread_t tid;
  //atomic_int* counter_ptr;
    //std::atomic<long int>* counter_ptr;
  };


struct arg_struct  args[2];

 void pin_this_thread_to_core(int core_id,pid_t tid) 
 {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
 
  if (core_id < 0 || core_id >= num_cores)
    {  printf("Error in CoreID\n");
        exit(0);

     }  //return EINVAL;

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


void* func_inc(void* )
{

    while(start.load()==0);

    #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif


   if(pthread_equal(threads[0],pthread_self()))
    {    
      pin_this_thread_to_core(0,tid);

    }

   if(pthread_equal(threads[1],pthread_self()))
    {   
      pin_this_thread_to_core(1,tid);
     }
  
    asm volatile ("" : : : "memory");
   __asm__ __volatile__ ( "mfence" ::: "memory" );


    pthread_t id = pthread_self();

    printf("Thread %u in func_inc\n",id);

    
   if(pthread_equal(id,threads[0]))
    { 
          
            printf("\n First thread processing...\n");
       
            if(counter.load()%2==0)
             { //atomic_fetch_add((args->counter_ptr),1);
                counter.fetch_add(1, std::memory_order_seq_cst);
                printf("Thread1 Counter %ld\n",counter.load());
             }
             else
             {
                   
               spin_thread_1++;
             }
         
    }
    else
       { 


           printf("\n Second thread processing...\n");
           
           if(counter.load()%2==1)
            {  
                counter.fetch_add(1, std::memory_order_seq_cst);
                printf("Thread2 Counter %ld\n",counter.load());
             }
             else
             {
              spin_thread_2++;
             } 
          

        }
    
    return NULL;
}


int main()
{
  start.store(0);
  int i = 0;
  int err;
  //counter=0;
  //args[0].counter_ptr=&counter;
  //args[1].counter_ptr=&counter;
   //fences required so that the above stores have happened before proceeding
   asm volatile ("" : : : "memory");
   __asm__ __volatile__ ( "mfence" ::: "memory" );

  

   for (i = 0; i < 2; i++)
     {
        err = pthread_create(&(threads[i]), NULL, &func_inc,NULL); //(void*)&args[i]);
        if (err != 0)
            printf("\n Can't create thread :[%s]\n", strerror(err));
        else
            printf("\nParent created Thread %d created successfully with thread ID %u.\n", i,threads[i]);
    }

   // pin_this_thread_to_core(threads[0],0);
   // pin_this_thread_to_core(threads[1],1);
   
    asm volatile ("mfence" : : : "memory");
    __asm__ __volatile__ ( "mfence" ::: "memory" );
 
    sleep(2);

     start.store(1); //all threads start executing now

       for (i = 0; i < 2; i++)     //waiting for threads to finish
        pthread_join(threads[i], NULL);


    printf("Unsuccessful spins Thread1 %ld Thread2 %ld\n",spin_thread_1,spin_thread_2);
    

return 0;
}


