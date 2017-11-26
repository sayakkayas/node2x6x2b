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

static long perf_event_open(struct perf_event_attr * hw_event,pid_t pid,int cpu,int group_fd,unsigned long flags)
{
  int ret;
  ret=syscall(__NR_perf_event_open,hw_event,pid,cpu,group_fd,flags);

  return ret;

}

#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define mb() asm volatile("mfence":::"memory")

#define BUFF_MAX 500
#define ITER 100000
/*size of int is 4 bytes, so each row has 16 integers=16*4=64 Bytes size of cache line at L1,
 *  so buff[BUFF_MAX][16] should occupy BUFF_MAX number of cache line as each row occupies a cache line*/
//Producer writes into buffer,consumer reads from it

//int buff[BUFF_MAX][16];

//in and out maybe on the same or different cache lines
//producer reads and writes 'in',consumer just reads it
 int in[16] ;

//consumer reads and writes 'out',Producer just reads it
int out[16];

pthread_t threads[2];

//pin threads to cores
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

  
  
   #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

     //pin producer to core 0  
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
   
    //  buff[in[0]][0] = item;
   

            //store in               
           in[0] = (in[0] + 1) % BUFF_MAX;
         
            count++;
   }


}


void* Consumer(void*)
{


  #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

     //pin consumer to core 1
     pin_this_thread_to_core(1,tid);
 

  int count=0;
     

 
  while (count<ITER)
   {
           //load in and out
   	while (in[0]== out[0]);

                 //load out
                 int w=out[0];
              
             //   int w=buff[out[0]][0];
           // printf("Consumed item %d\n",w);
                    
         //store out
        out[0] = (out[0] + 1) % BUFF_MAX;
      
      count++;
   }

}


int main()
{


int i,err;

in[0]=0;
out[0]=0;
  //initialize Perf event parameters
//Counting event remote HITM 
  struct perf_event_attr pe;   //for event count on core 0
  struct perf_event_attr pe2;  //for event count on core 1
  long long count;            //counts and stores  events on core 0 
  long long count2;           //counts and stores events on core 1

  memset(&pe,0,sizeof(struct perf_event_attr));   
  memset(&pe2,0,sizeof(struct perf_event_attr));

  pe.type=PERF_TYPE_RAW;
  pe.size=sizeof(struct perf_event_attr);
  pe.config=0x10d3;
  pe.disabled=1;
  pe.exclude_kernel=1;
  pe.exclude_hv=1;


    pe2.type=PERF_TYPE_RAW;
    pe2.size=sizeof(struct perf_event_attr);
    pe2.config=0x10d3;
    pe2.disabled=1;
    pe2.exclude_kernel=1;
  pe2.exclude_hv=1;

 
 int fd;
 fd=perf_event_open(&pe,-1,0,-1,0);

 int fd2;
 fd2=perf_event_open(&pe2,-1,1,-1,0);
 

 if(fd==-1 || fd2==-1 )
 { 
   printf("Error opening leader %llx  %llx\n",pe.config,pe2.config);
     exit(EXIT_FAILURE);
  }


  //start event counting
  ioctl(fd,PERF_EVENT_IOC_RESET,0);
   ioctl(fd,PERF_EVENT_IOC_ENABLE,0);
  
   ioctl(fd2,PERF_EVENT_IOC_RESET,0);
   ioctl(fd2,PERF_EVENT_IOC_ENABLE,0);
   


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

 
    for (i = 0; i < 2; i++)
        pthread_join(threads[i], NULL);

ioctl(fd2,PERF_EVENT_IOC_DISABLE,0);
  ioctl(fd,PERF_EVENT_IOC_DISABLE,0);
  read(fd,&count,sizeof(long long));
  read(fd2,&count2,sizeof(long long));
  printf("EVENT remote cache HITM on core 0 : %lld\n",count);
  printf("EVENT remote cache HITM on core 1 : %lld\n",count2);
    close(fd);
    close(fd2);


	return 0;
}


