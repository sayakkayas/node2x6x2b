#include <asm/unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

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

#define PAGE_COUNT 8
#define PAGE_SIZE getpagesize()
#define BYTE_COUNT (PAGE_COUNT * PAGE_SIZE)
#define SAMPLE_PERIOD 100

struct perf_event_mmap_page *map_page1;
struct perf_event_mmap_page *map_page2;


long long prev_head = 0;
unsigned char *data = NULL;

long long prev_head2 = 0;
unsigned char *data2 = NULL;


static long perf_event_open(struct perf_event_attr * hw_event,pid_t pid,int cpu,int group_fd,unsigned long flags)
{
  int ret;
  ret=syscall(__NR_perf_event_open,hw_event,pid,cpu,group_fd,flags);

  return ret;

}


void count_events()
{

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


}


static void our_handler2(int signum, siginfo_t *info, void *uc) 
{

printf("our_handler2\n");
  int ret;
  struct perf_event_mmap_page *control_page = map_page2;

  void *data_mmap = (void *)((unsigned long)map_page2 + PAGE_SIZE);
  
  long long head, size, prev_head_wrap;

  int fd = info->si_fd;

  ret = ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

  head = control_page->data_head;
  size = head - prev_head2;

  if(size > BYTE_COUNT) 
  {
    printf("overflow!\n");
  }

  prev_head_wrap = prev_head % BYTE_COUNT;

  prev_head2 = control_page->data_head;

  if(!data2)
  { data2 = malloc(BYTE_COUNT);
   }

  if(size)
  {
    unsigned long offset = 0, loffset = 0;
    unsigned long ip, addr,d_addr;
    int pid, tid,weight;
    struct perf_event_header *event;

    memcpy(data,(unsigned char *)data_mmap+prev_head_wrap,BYTE_COUNT - prev_head_wrap);

    memcpy(data+(BYTE_COUNT-prev_head_wrap),(unsigned char *)data_mmap,prev_head_wrap);

    printf("---------------\n");

    while (offset < size)
     {
      event = (struct perf_event_header *)&data[offset];

      loffset = offset + 8;  // Skip the header.

      memcpy(&ip, &data[loffset], sizeof(unsigned long));
      loffset += 8;

      memcpy(&pid, &data[loffset], sizeof(int));
      memcpy(&tid, &data[loffset + 4], sizeof(int));
      loffset += 8;

      memcpy(&addr, &data[loffset], sizeof(unsigned long));
      loffset += 8;
      
       memcpy(&weight, &data[loffset], sizeof(unsigned long));
      loffset += 8;

       memcpy(&d_addr, &data[loffset], sizeof(unsigned long));
      loffset += 8;


      printf("pid: %d tid: %d ip: %p addr: %p weight:%d Daddr:%p\n", pid, tid, (void *)ip, (void *)addr,weight,(void*)d_addr);

      offset += event->size;
    }
  }

  ret = ioctl(fd, PERF_EVENT_IOC_REFRESH, 1);

  (void)ret;
}



static void our_handler1(int signum, siginfo_t *info, void *uc) 
{

 printf("our_handler1\n");
  int ret;
  struct perf_event_mmap_page *control_page = map_page1;

  void *data_mmap = (void *)((unsigned long)map_page1 + PAGE_SIZE);
  
  long long head, size, prev_head_wrap;

  int fd = info->si_fd;

  ret = ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

  head = control_page->data_head;
  size = head - prev_head;

  if(size > BYTE_COUNT) 
  {
    printf("overflow!\n");
  }

  prev_head_wrap = prev_head % BYTE_COUNT;

  prev_head = control_page->data_head;

  if(!data)
  { data = malloc(BYTE_COUNT);
   }

  if(size)
  {
    unsigned long offset = 0, loffset = 0;
    unsigned long ip, addr,d_addr;
    int pid, tid,weight;
    struct perf_event_header *event;

    memcpy(data,(unsigned char *)data_mmap+prev_head_wrap,BYTE_COUNT - prev_head_wrap);

    memcpy(data+(BYTE_COUNT-prev_head_wrap),(unsigned char *)data_mmap,prev_head_wrap);

    printf("---------------\n");

    while (offset < size)
     {
      event = (struct perf_event_header *)&data[offset];

      loffset = offset + 8;  // Skip the header.

      memcpy(&ip, &data[loffset], sizeof(unsigned long));
      loffset += 8;

      memcpy(&pid, &data[loffset], sizeof(int));
      memcpy(&tid, &data[loffset + 4], sizeof(int));
      loffset += 8;

      memcpy(&addr, &data[loffset], sizeof(unsigned long));
      loffset += 8;
      
       memcpy(&weight, &data[loffset], sizeof(unsigned long));
      loffset += 8;

       memcpy(&d_addr, &data[loffset], sizeof(unsigned long));
      loffset += 8;


      printf("pid: %d tid: %d ip: %p addr: %p weight:%d Daddr:%p\n", pid, tid, (void *)ip, (void *)addr,weight,(void*)d_addr);

      offset += event->size;
    }
  }

  ret = ioctl(fd, PERF_EVENT_IOC_REFRESH, 1);

  (void)ret;
}


void sample()
{

	//time for sampling set up

	long long count=0;
	long long count2=0;

 int millisec=100;
 struct timespec tim,tim2;
 tim.tv_sec=millisec/1000;
 tim.tv_nsec=(millisec%1000) * 1000000 ;




  //sampling
//coherence event threshold 572,000 events per second
//so for 0.1 second it is 57,200 events
//if sampling period is sample every 100 events then we should get a total of 572 samples


  struct perf_event_attr  pe;
  struct perf_event_attr  pe2;
  struct sigaction sa;
  struct sigaction sa2;

  printf("1\n");
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = our_handler1;
  sa.sa_flags = SA_SIGINFO;
  
  memset(&sa2, 0, sizeof(struct sigaction));
  sa2.sa_sigaction = our_handler2;
  sa2.sa_flags = SA_SIGINFO;


  if (sigaction(SIGIO, &sa, NULL) < 0 || sigaction(SIGIO, &sa2, NULL) < 0) 
  {
    fprintf(stderr, "Error setting up signal handler\n");
    exit(EXIT_FAILURE);
  }

  
  memset(&pe,0,sizeof(struct perf_event_attr));
  pe.type=PERF_TYPE_RAW;
  pe.size=sizeof(struct perf_event_attr);
  pe.config=0x10d3;
  pe.sample_period=SAMPLE_PERIOD;
  pe.sample_type=PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_ADDR | PERF_SAMPLE_WEIGHT|  PERF_SAMPLE_DATA_SRC;
  pe.freq=0; //no requency
  pe.mmap=1;
  pe.mmap_data=1;
  
  pe.read_format = 0;
  pe.pinned = 1;
  pe.mmap = 1;
  pe.task = 1;
  pe.wakeup_events = 1;
  pe.precise_ip = 2;

  pe.task=1; 
  pe.exclude_kernel=1;
  pe.exclude_hv=1;
  pe.disabled=1;
  pe.sample_id_all=1; 
  int fd;


    memset(&pe2,0,sizeof(struct perf_event_attr));
  pe2.type=PERF_TYPE_RAW;
  pe2.size=sizeof(struct perf_event_attr);
  pe2.config=0x10d3;
  pe2.sample_period=SAMPLE_PERIOD;
  pe2.sample_type=PERF_SAMPLE_TID | PERF_SAMPLE_RAW | PERF_SAMPLE_DATA_SRC | PERF_SAMPLE_ADDR;
  pe2.freq=0; //no requency
  pe2.mmap=1;
  pe2.mmap_data=1;
  
  pe2.read_format = 0;
  pe2.pinned = 1;
  pe2.mmap = 1;
  pe2.task = 1;
  pe2.wakeup_events = 1;
  pe2.precise_ip = 2;

  pe2.task=1; 
  pe2.exclude_kernel=1;
  pe2.exclude_hv=1;
  pe2.disabled=1;
  pe2.sample_id_all=1; 
  int fd2;

 //sample on core 0

  fd=perf_event_open(&pe,-1,0,-1,PERF_FLAG_FD_OUTPUT);
  fd2=perf_event_open(&pe2,-1,1,-1,PERF_FLAG_FD_OUTPUT);
 
  if(fd==-1 || fd2==-1)
   {
      printf("Error openong leadder %llx\n",pe.config);
      exit(EXIT_FAILURE);
   }


 map_page1=mmap(NULL,(PAGE_COUNT + 1) * PAGE_SIZE,PROT_WRITE | PROT_READ,MAP_SHARED,fd,0);
 map_page2=mmap(NULL,(PAGE_COUNT + 1) * PAGE_SIZE,PROT_WRITE | PROT_READ,MAP_SHARED,fd2,0);

   printf("2\n");
   fcntl(fd, F_SETFL, O_RDWR | O_NONBLOCK | O_ASYNC);
   fcntl(fd, F_SETOWN, getpid());
  
   printf("3\n");
    fcntl(fd2,F_SETFL,O_RDWR |O_NONBLOCK |O_ASYNC);
    fcntl(fd2,F_SETOWN,getpid());

  printf("4\n");
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
  
  ioctl(fd2, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd2, PERF_EVENT_IOC_ENABLE, 0);

   nanosleep(&tim,NULL);
 
   //nanosec

  ioctl(fd, PERF_EVENT_IOC_REFRESH, 0);
  ioctl(fd2, PERF_EVENT_IOC_REFRESH, 0);

  read(fd, &count, sizeof(long long));
  read(fd2, &count2, sizeof(long long));

  printf("Core 0 %lld  HITM events occurred.\n", count);
  printf("Core 1 %lld  HITM events occurred.\n", count2);

    munmap(map_page1, (PAGE_COUNT + 1) * PAGE_SIZE);
    munmap(map_page2, (PAGE_COUNT + 1) * PAGE_SIZE);

  close(fd);
 close(fd2);



}

int main()
{

 count_events();
  sample(); 
return 0;
}
