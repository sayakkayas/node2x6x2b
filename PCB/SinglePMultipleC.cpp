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

#define BUFF_MAX 200
#define ITER 1000
/*size of int is 4 bytes, so each row has 16 integers=16*4=64 Bytes size of cache line at L1,
 *  so buff[BUFF_MAX][16] should occupy BUFF_MAX number of cache line as each row occupies a cache line*/
//Producer writes into buffer,consumer reads from it


 //single buffer
 int buff[BUFF_MAX][16];

//in and out maybe on the same or different cache lines
//producer reads and writes 'in',consumer just reads it

 //single  in as one Producer ,need not need to be atomic as no data race(only one Producer modifies it) 
// int in[16];
std::atomic<long int> in= ATOMIC_VAR_INIT(0);

//consumer reads and writes 'out',Producer just reads it
 //Atomic out as multiple consumer modifies it and thus data race possible
std::atomic<long int> out = ATOMIC_VAR_INIT(0);

pthread_t* threads;
 struct perf_event_attr* pe;   //for perf event 
  long long* count;            //counts and stores  events on core 0 
 int* fd; //file pointers returned from perf event open


//pin threads to cores
 void pin_this_thread_to_core(int core_id,pid_t tid,int i) 
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
           printf("Producer %d Thread %u assigned to core %d\n",i,tid,core_id);
        }
        else
        {
          if(CPU_ISSET(core_id,&cpuset[core_id]))
          printf("Consumer %d Thread %u assigned to core %d\n",i,tid,core_id);

        }
     }
     else
      {
        printf("Failed to pin thread %u on core %d\n",tid,core_id);

       }

}



void initialize(int n)
{
	int i;

	 //B=(struct buf*)malloc(n * sizeof(struct buf));
 
    threads=(pthread_t*)malloc(n*sizeof(pthread_t));

         

}



void initialize_perf(int n)
{

  //initialize Perf event parameters
//Counting event remote HITM 
	 pe=(struct perf_event_attr*)malloc(n*sizeof(struct perf_event_attr));
   count=(long long*)malloc(n*sizeof(long long)); 

   int i;
   for(i=0;i<n;i++)
   {
    memset(&pe[i],0,sizeof(struct perf_event_attr));   
   	
   }

   fd=(int*)malloc(n*sizeof(int));

 for(i=0;i<n;i++)
 {
  pe[i].type=PERF_TYPE_RAW;
  pe[i].size=sizeof(struct perf_event_attr);
  pe[i].config=0x10d3;
  pe[i].disabled=1;
  pe[i].exclude_kernel=1;
  pe[i].exclude_hv=1;
 }
   


  for(i=0;i<n;i++)
  {
    fd[i]=perf_event_open(&pe[i],-1,i,-1,0);
 
    if(fd[i]==-1 )
     { 
       printf("Error opening leader %llx \n",pe[i].config);
        exit(EXIT_FAILURE);
     }


  }

 //initialize counters
  for(i=0;i<n;i++)
  {
  	count[i]=0;
  }
 

  //start event counting
  for(i=0;i<n;i++)
  {
    ioctl(fd[i],PERF_EVENT_IOC_RESET,0);
   ioctl(fd[i],PERF_EVENT_IOC_ENABLE,0);
  	
  }
  
  

}

struct para
{
 int max;
 int i;

};




void terminate_perf(int n)
{
  int i;

   for(i=0;i<n;i++)
  {
   ioctl(fd[i],PERF_EVENT_IOC_DISABLE,0);
 	
  }


}


void* Producer(void* i)
{

  int index=*((int *)i);
 // printf("Producer index %d\n",index);
   
   #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

     //pin producer to core 0  
     pin_this_thread_to_core(0,tid,index);
  
   int count=0;


  while (count<ITER)
   {
       //produce item  
   	   int item=rand()%100;

          //load in and out
      while ((in.load(std::memory_order_acquire) + 1) % BUFF_MAX == out.load(std::memory_order_acquire)); //do nothing
 
       //store buffer
       
        int w=in.load(std::memory_order_acquire); 
   
    //  buff[in[0]][0] = item;
   

            //store in               
          // in[0] = (in[0] + 1) % BUFF_MAX;
         
            in.fetch_add(1, std::memory_order_release);
          in.store(in.load(std::memory_order_acquire)%BUFF_MAX,std::memory_order_release);


            count++;
   }


}


void* Consumer(void* arg)
{
 struct para* ARG=(struct para*)arg;
  int index=ARG->i;
  int max=ARG->max;
// printf("consumer index  %d consumer max %d\n",index,max);
 
  #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

     //pin consumer to core 1
     pin_this_thread_to_core(index,tid,index);
  // printf("Consumer %d\n",index);

  int count=0;
      
  while (count<(ITER/max))
   {
           //load in and out
   	  while (in.load(std::memory_order_acquire)== out.load(std::memory_order_acquire));

                 //load out
                 int w=out.load(std::memory_order_acquire);
              
             //   int w=buff[out.load(std::memory_order_acquire)][0];
           // printf("Consumed item %d\n",w);
                    
         //store out
          out.fetch_add(1, std::memory_order_release);
          out.store(out.load(std::memory_order_acquire)%BUFF_MAX,std::memory_order_release);
        
      count++;
   }

}



void display_count(int n)
{

  int i;
   for(i=0;i<n;i++)
   {
	    read(fd[i],&count[i],sizeof(long long));
       printf("EVENT remote cache HITM on core %d : %lld\n",i,count[i]);
       close(fd[i]);
   }
}




int main(int argc,char* argv[])
{

  if(argc==2)
  {
     int n= atoi(argv[1]);
      initialize(n);
      initialize_perf(n);	

      int i,err,err2;
       //   in[0]=0;
   
     for(i=0;i<n;i++)
       {

             int *arg =(int*)malloc(sizeof(*arg));
              struct para * ARG =(struct para*)malloc(sizeof(struct para));
      

           if ( arg == NULL  )
             { 
          		  fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
          	          exit(EXIT_FAILURE);
               }

            *arg = i;
            ARG->max=n-1;
           ARG->i=i;
         
          if(i==0)
           { 
             err = pthread_create(&(threads[i]), NULL, &Producer,arg); 
             
             if (err != 0)
              printf("\n Can't create thread :[%s]\n", strerror(err));
             // else
             // printf("\n Thread %d created successfully with thread ID %u.\n", i,threads[i]);
           }
           else
            {
              err2 = pthread_create(&(threads[i]), NULL, &Consumer,ARG);
         
              if (err2 != 0)
               printf("\n Can't create thread :[%s]\n", strerror(err2));
             // else
             // printf("\n Thread %d created successfully with thread ID %u.\n", i,threads[i]);
             }
          //producer consumer invokation goes here
       }

      
           for (i = 0; i < n; i++)
           { pthread_join(threads[i], NULL);
              
           }

         

      terminate_perf(n);
      display_count(n);
    

  }
  else
  {
  	printf("No parameters\n");
  }

 return 0;
}
