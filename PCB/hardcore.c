#include <stdio.h>
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

int main()
{

 //time for sampling set up
 int millisec=100;
 struct timespec tim,tim2;
 tim.tv_sec=millisec/1000;
 tim.tv_nsec=(millisec%1000) * 1000000 ;

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
//pe.sample_freq=1000;
//pe.sample_type=PERF_SAMPLE_TID ;
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

   ioctl(fd,PERF_EVENT_IOC_RESET,0);
   ioctl(fd,PERF_EVENT_IOC_ENABLE,0);
  
   ioctl(fd2,PERF_EVENT_IOC_RESET,0);
   ioctl(fd2,PERF_EVENT_IOC_ENABLE,0);
    //system wide for two seconds
     sleep(5);
   // nanosleep(&tim,NULL);
 
  ioctl(fd2,PERF_EVENT_IOC_DISABLE,0);
  
  ioctl(fd,PERF_EVENT_IOC_DISABLE,0);
    

     read(fd,&count,sizeof(long long));
     read(fd2,&count2,sizeof(long long));

     printf("EVENT remote cache HITM on core 0 : %lld\n",count);
     printf("EVENT remote cache HITM on core 1 : %lld\n",count2);
    close(fd);
    close(fd2);

  //sampling
//coherence event threshold 572,000 events per second
//so for 0.1 second it is 57,200 events
//if sampling period is sample every 100 events then we should get a total of 572 samples


  struct perf_event_attr  pe3;
  memset(&pe3,0,sizeof(struct perf_event_attr));
  pe3.type=PERF_TYPE_RAW;
  pe3.size=sizeof(struct perf_event_attr);
  pe3.config=0x10d3;
 // pe2.freq=1;
  pe3.sample_period=100;
  pe3.sample_type=PERF_SAMPLE_TID | PERF_SAMPLE_RAW | PERF_SAMPLE_DATA_SRC | PERF_SAMPLE_ADDR;
  pe3.freq=0; //no requency
  pe3.mmap=1;
  pe3.mmap_data=1;
  pe3.precise_ip=2;
  pe3.task=1; 
  pe3.exclude_kernel=1;
  pe3.exclude_hv=1;
  pe3.disabled=1;
  pe3.sample_id_all=1; 
  int fd3;

 //sample on core 0

  fd3=perf_event_open(&pe3,-1,0,-1,PERF_FLAG_FD_OUTPUT);
 
  if(fd3==-1)
   {
      printf("Error openong leadder %llx\n",pe3.config);
      exit(EXIT_FAILURE);
   }

 // mmap(fd);
  //MAp result,size should be 1+2^n

 size_t mmap_pages_count=2048;
 size_t p_size=sysconf(_SC_PAGESIZE);
 printf("p_size %d \n",p_size);
 size_t mmap_len=p_size*(1+mmap_pages_count);
 printf("mmap_len %d\n",mmap_len);
 struct perf_event_mmap_page * metadata=mmap(NULL,mmap_len,PROT_WRITE | PROT_READ,MAP_SHARED,fd3,0);

   printf("metadata %p\n",metadata);
   printf("metadata->data_head  %" PRIu64 "\n",metadata->data_head);
 

  //start sampline
   ioctl(fd3,PERF_EVENT_IOC_RESET,0);
   ioctl(fd3,PERF_EVENT_IOC_ENABLE,0);
   
  //collect samples for 0.1 seconds

   nanosleep(&tim,NULL);
   //sleep(1);

    ioctl(fd3,PERF_EVENT_IOC_DISABLE,0);
 
    // close(fd3);
  
   //read mmap buffer   
   //Analyse samples
   uint64_t head_samp=metadata->data_head;
 
   //printf("data size %" PRIu64 "\n",metadata->data_size); 
    printf("metadata->data_head  %" PRIu64 "\n",head_samp);
    // printf("metadata->data_tail %" PRIu64 "\n",metadata->data_tail);

    rmb();

  //skiping first metadata page
   
  struct perf_event_header * header;
 // memset(header,0,sizeof(struct perf_event_header));
      header=metadata+p_size;
  
    uint64_t consumed=0;
  

  // printf("Head_samp %" PRIu64 "\n",head_samp);
  // printf("consumed %" PRIu64 "\n",consumed);
   printf("header %p\n",header);
   //header=header+8;
   printf("header->size %" PRIu16 "\n",header->size);
   printf("header->type %" PRIu32 "\n",header->type);
   printf("header->misc %" PRIu16 "\n",header->misc);
  
   exit(0);
   int counter_while=0; 

 while(consumed < head_samp)
 {

        counter_while++;
      
         if(header->type==PERF_RECORD_SAMPLE)
          {
              printf("Yes Record sample\n");
          }
           
              //printf("consumed in loop %" PRIu64 "\n",consumed);
             //printf("header->size %" PRIu16 "\n",(uint16_t)header->size);
           consumed=consumed+header->size;
          printf("consumed in loop %" PRIu64 "\n",consumed);

       header=(struct perf_event_header *) (((char*)header) +(header->size) );

  }//while closed

close(fd3);

return 0;
}
