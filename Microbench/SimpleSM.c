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

#define _STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#define rmb() asm volatile("lfence":::"memory")
#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define mb() asm volatile("mfence":::"memory")
#define ITER 10000

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



void* create_shared_memory(size_t size) 
{
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_ANONYMOUS | MAP_SHARED;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  return mmap(NULL, size, protection, visibility, 0, 0);
}



int main() 
{
  int data=2;

  void* shmem = create_shared_memory(128);

 // memcpy(shmem, parent_message, sizeof(parent_message));

  int pid = fork();

  if (pid == 0) 
  { 
  	#ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

  	pin_this_thread_to_core(1,tid);
    sleep(1);
    
    int i=0;
     while(ITER>i)
     {
        printf("Child read: %s\n", shmem);
    
      	i++;
     }

  } 
  else 
  {
  	#ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

  	pin_this_thread_to_core(0,tid);

       int i=0;
     while(ITER>i)
     {
        memcpy(shmem, data, sizeof(data));
    
      	i++;
     }


  }
}