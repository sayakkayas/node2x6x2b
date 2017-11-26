#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <stdlib.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/mman.h>
#include <asm/unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <stdlib.h>
#include <linux/perf_event.h>
//#include<linux/perf_helpers.h>
#include <linux/hw_breakpoint.h>
#include <sys/mman.h>
#define _STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define rmb() asm volatile("lfence":::"memory")
#define ITER 1000

//pin threads to cores
void pin_this_thread_to_core(int core_id,pid_t tid) 
 {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
 
  if (core_id < 0 || core_id >= num_cores)
    {  printf("Error in CoreID\n");
        exit(0);

     }  //return EINVAL;

   cpu_set_t cpuset[40];
   CPU_ZERO(&cpuset[core_id]);
   CPU_SET(core_id, &cpuset[core_id]);

   pthread_t current_thread = pthread_self();    
   int no_err= pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset[core_id]);

   if(no_err==0)//success
    {
        if(core_id==0)
        {
          if(CPU_ISSET(core_id,&cpuset[core_id]))
           printf("Parent %d assigned to core %d\n",tid,core_id);
        }
        else
        {
          if(CPU_ISSET(core_id,&cpuset[core_id]))
          printf("Child %d assigned to core %d\n",tid,core_id);

        }
     }
     else
      {
        printf("Failed to pin thread %u on core %d\n",tid,core_id);

       }

}



int main()
{
 
  int   shm_id;        /* shared memory ID      */
  int* array;
  key_t key=12345;
  shm_id = shmget(key, 16*sizeof(int), IPC_CREAT );

  if(shm_id < 0)
  {
     printf("shmget error\n");
     exit(1);
  }

  array = (int *)shmat(shm_id, 0, 0);

   array = malloc(sizeof(int)*16);



  if(fork()==0)
  { 
  	#ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif
  	pin_this_thread_to_core(1,tid);
  

    //sleep(1);
   int c=0;
    while(ITER>c)
    { //read from data
          printf("%d\n",array[0]);
    	c++;
    }


    exit(0);
  }
  else
  {

  	#ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif
  	pin_this_thread_to_core(0,tid);
   
     int c=0;
    while(ITER>c)
    { 
    	//write
    	int r=rand()%100;
        array[0]=r;
     	c++;
    }

  }

wait(NULL);
  
	return 0;

}