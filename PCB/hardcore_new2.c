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

#define rmb() asm volatile("lfence":::"memory")

#define PAGE_COUNT 64
#define PAGE_SIZE getpagesize()
#define BYTE_COUNT (PAGE_COUNT * PAGE_SIZE)
#define SAMPLE_PERIOD 1000

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



void findSource(char* dSource,long long src)
{


   if(src!=0)
   strcpy(dSource,"");

   if (src & (PERF_MEM_OP_NA<<PERF_MEM_OP_SHIFT))
     strcpy(dSource,"Op Not available ");

   if (src & (PERF_MEM_OP_LOAD<<PERF_MEM_OP_SHIFT))
            strcpy(dSource,"Load ");

          if (src & (PERF_MEM_OP_STORE<<PERF_MEM_OP_SHIFT))
            strcpy(dSource,"Store ");

          if (src & (PERF_MEM_OP_PFETCH<<PERF_MEM_OP_SHIFT))
            strcpy(dSource,"Prefetch ");

          if (src & (PERF_MEM_OP_EXEC<<PERF_MEM_OP_SHIFT))
            strcpy(dSource,"Executable code ");

          if (src & (PERF_MEM_LVL_NA<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Level Not available ");

          if (src & (PERF_MEM_LVL_HIT<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Hit ");

          if (src & (PERF_MEM_LVL_MISS<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Miss ");

          if (src & (PERF_MEM_LVL_L1<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"L1 cache ");

          if (src & (PERF_MEM_LVL_LFB<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Line fill buffer ");
          
          if (src & (PERF_MEM_LVL_L2<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"L2 cache ");

          if (src & (PERF_MEM_LVL_L3<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"L3 cache ");

          if (src & (PERF_MEM_LVL_LOC_RAM<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Local DRAM ");

          if (src & (PERF_MEM_LVL_REM_RAM1<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Remote DRAM 1 hop ");

          if (src & (PERF_MEM_LVL_REM_RAM2<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Remote DRAM 2 hops ");

          if (src & (PERF_MEM_LVL_REM_CCE1<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Remote cache 1 hop ");

         if (src & (PERF_MEM_LVL_REM_CCE2<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Remote cache 2 hops ");

          if (src & (PERF_MEM_LVL_IO<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"I/O memory ");

          if (src & (PERF_MEM_LVL_UNC<<PERF_MEM_LVL_SHIFT))
            strcpy(dSource,"Uncached memory ");

          if (src & (PERF_MEM_SNOOP_NA<<PERF_MEM_SNOOP_SHIFT))
            strcpy(dSource,"Not available ");

          if (src & (PERF_MEM_SNOOP_NONE<<PERF_MEM_SNOOP_SHIFT))
            strcpy(dSource,"No snoop ");

          if (src & (PERF_MEM_SNOOP_HIT<<PERF_MEM_SNOOP_SHIFT))
            strcpy(dSource,"Snoop hit ");

          if (src & (PERF_MEM_SNOOP_MISS<<PERF_MEM_SNOOP_SHIFT))
            strcpy(dSource,"Snoop miss ");

          if (src & (PERF_MEM_SNOOP_HITM<<PERF_MEM_SNOOP_SHIFT))
            strcpy(dSource,"Snoop hit modified ");

          if (src & (PERF_MEM_LOCK_NA<<PERF_MEM_LOCK_SHIFT))
            strcpy(dSource,"Not available ");

          if (src & (PERF_MEM_LOCK_LOCKED<<PERF_MEM_LOCK_SHIFT))
            strcpy(dSource,"Locked transaction ");

          if (src & (PERF_MEM_TLB_NA<<PERF_MEM_TLB_SHIFT))
            strcpy(dSource,"Not available ");

          if (src & (PERF_MEM_TLB_HIT<<PERF_MEM_TLB_SHIFT))
            strcpy(dSource,"TLB Hit ");

          if (src & (PERF_MEM_TLB_MISS<<PERF_MEM_TLB_SHIFT))
            strcpy(dSource,"TLB Miss ");

          if (src & (PERF_MEM_TLB_L1<<PERF_MEM_TLB_SHIFT))
            strcpy(dSource,"Level 1 TLB ");

          if (src & (PERF_MEM_TLB_L2<<PERF_MEM_TLB_SHIFT))
            strcpy(dSource,"Level 2 TLB ");

          if (src & (PERF_MEM_TLB_WK<<PERF_MEM_TLB_SHIFT))
            strcpy(dSource,"Hardware walker ");

          if (src & ((long long)PERF_MEM_TLB_OS<<PERF_MEM_TLB_SHIFT))
            strcpy(dSource,"OS fault handler ");
 

}


static void print_event_type(unsigned int event,int sample_type)
{
   switch(event)
   {

     case PERF_RECORD_MMAP:printf("PERF_RECORD_MMAP\n");
                           break;
   
    case PERF_RECORD_LOST:printf("PERF_RECORD_LOST\n");
                           break;
     case PERF_RECORD_COMM:printf("PERF_RECORD_COMM\n");
                           break;
    case PERF_RECORD_EXIT:printf("PERF_RECORD_EXIT\n");
                           break;
     case PERF_RECORD_THROTTLE:printf("PERF_RECORD_THROTTLE\n");
                           break;

     case PERF_RECORD_UNTHROTTLE:printf("PERF_RECORD_UNTHROTTLE\n");
                           break;
      case PERF_RECORD_FORK:printf("PERF_RECORD_FORK\n");
                           break;
      case PERF_RECORD_READ:printf("PERF_RECORD_READ\n");
                           break;

      case PERF_RECORD_SAMPLE:printf("PERF_RECORD_SAMPLE [%x]\n",sample_type);
                           break;

       case PERF_RECORD_MMAP2:printf("PERF_RECORD_MMAP2\n");
                           break;
     /*
       case PERF_RECORD_AUX:printf("PERF_RECORD_AUX\n");
                           break;

       case PERF_RECORD_ITRACE_START:printf("PERF_RECORD_ITRACE_START\n");
                           break;


       case PERF_RECORD_LOST_SAMPLES:printf("PERF_RECORD_LOST_SAMPLES\n");
                         break;

       case PERF_RECORD_SWITCH:printf("PERF_RECORD_SWITCH\n");
                         break;
     
         case PERF_RECORD_SWITCH_CPU_WIDE:printf("PERF_RECORD_SWITCH_CPU_WIDE\n");
                         break;

   */

     default:printf("Unknown\n");
            break;
   }



}



static int perf_mmap_read(void* our_mmap,int mmap_size,long long prev_head,int sample_type,int read_format) 
{
  int ret;
  struct perf_event_mmap_page *control_page = our_mmap;

  long long head,offset, size,bytesize, prev_head_wrap;
  int i;


  unsigned char* data;
  void *data_mmap = our_mmap+getpagesize();
  
// printf("1\n");  
   if(mmap_size==0)
    {
       printf("MMAP SIZE 0\n");
      return 0;
    }

    if(control_page==NULL)
   {
     fprintf(stderr,"ERROR mmap page NULL\n");
     return -1;

   }
 
// printf("2\n");

  head = control_page->data_head;
  rmb(); //fence
  size = head - prev_head;

  bytesize=mmap_size*getpagesize();

  if(size > bytesize) 
  {
    printf("overflow!\n");
  }

   //

  //printf("3\n");
 data = malloc(bytesize);
 prev_head_wrap = prev_head % bytesize;

//
 prev_head = control_page->data_head;

// printf("4\n");
 
  if(size)
  {
  // printf("5\n");
    unsigned long offset = 0, loffset = 0;
    unsigned long ip, addr,d_addr;
    int pid, tid,weight;
    struct perf_event_header *event;

    memcpy(data,(unsigned char *)data_mmap+prev_head_wrap,bytesize - prev_head_wrap);

    memcpy(data+(bytesize-prev_head_wrap),(unsigned char *)data_mmap,prev_head_wrap);

    printf("---------------\n");

  int count=0;
    while (offset < size)
     {
      event = (struct perf_event_header *)&data[loffset];
      // ip=0;addr=0;d_addr=0;pid=0;tid=0;weight=0;

     //  print_event_type(event->type,sample_type);

      /*   loffset = offset + 8;  // Skip the header.

      //    printf("6\n");
       //new addition

       if(event->type==PERF_RECORD_SAMPLE)
       {
         //   printf("7\n");
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

   	 //  if(d_addr!=0)
     	  { printf("pid: %d tid: %d ip: %p addr: %p weight:%d Daddr:%p\n", pid, tid, (void *)ip, (void *)addr,weight,(void*)d_addr);
       		 count++;
    	  }
       }
 
      offset += event->size;
       */
        offset=loffset;
        offset+=8; //skip header
       if(event->type==PERF_RECORD_THROTTLE || event->type == PERF_RECORD_UNTHROTTLE)
        {
            long long throttle_time=0,id=0,stream_id=0;
         memcpy(&throttle_time,&data[offset],sizeof(long long));
      //   printf("Time:%lld ",throttle_time);
           offset+=8;
       
        memcpy(&id,&data[offset],sizeof(long long));
       // printf("ID:%lld ",id);
        offset+=8;   

        memcpy(&stream_id,&data[offset],sizeof(long long));
       // printf("STREAM ID:%lld\n",stream_id);
        offset+=8;

        }

     if(event->type==PERF_RECORD_SAMPLE)
      {
            long long ip=0; /* if PERF_SAMPLE_IP */
             int pid=0,tid=0; /* if PERF_SAMPLE_TID */
             long long time=0;/* if PERF_SAMPLE_TIME */
             long long addr=0; /* if PERF_SAMPLE_ADDR */
             long long id=0;   /* if PERF_SAMPLE_ID */
             long long  stream_id=0;  /* if PERF_SAMPLE_STREAM_ID */
             int  cpu=0, res=0;   /* if PERF_SAMPLE_CPU */
              long long   period=0;     /* if PERF_SAMPLE_PERIOD */
             // struct read_format v; /* if PERF_SAMPLE_READ */
              long long   nr=0;         /* if PERF_SAMPLE_CALLCHAIN */
              //long long   ips[1000];    /* if PERF_SAMPLE_CALLCHAIN */
                int   size_raw=0;       /* if PERF_SAMPLE_RAW */
                //char  data[size]; /* if PERF_SAMPLE_RAW */
                 long long   bnr=0;        /* if PERF_SAMPLE_BRANCH_STACK */
                //struct perf_branch_entry lbr[bnr];   /* if PERF_SAMPLE_BRANCH_STACK */
                 long long   abi=0;        /* if PERF_SAMPLE_REGS_USER */
          //long long  regs[weight(mask)]; /* if PERF_SAMPLE_REGS_USER */
          long long   size_stack=0;       /* if PERF_SAMPLE_STACK_USER */
         //char  data[size]; /* if PERF_SAMPLE_STACK_USER */
         //u64   dyn_size;   /* if PERF_SAMPLE_STACK_USER */
    
         long long  weight=0;     /* if PERF_SAMPLE_WEIGHT */
         long long   data_src=0;   /* if PERF_SAMPLE_DATA_SRC */
               char dataSource[100];
      
            if(sample_type & PERF_SAMPLE_IP)
            { //long long ip;
              memcpy(&ip,&data[offset],sizeof(long long));
             // printf("IP:%llx ",ip);
               offset+=8;

             }

             if(sample_type & PERF_SAMPLE_TID)
              { //int pid,tid;
                 memcpy(&pid,&data[offset],sizeof(int));
                 memcpy(&tid,&data[offset+4],sizeof(int));
               //  printf("PID:%d TID:%d ",pid,tid);
                 offset+=8;


             }

             if(sample_type & PERF_SAMPLE_TIME)
              { //int pid,tid;
                 memcpy(&time,&data[offset],sizeof(int));
               //  printf("PID:%d TID:%d ",pid,tid);
                 offset+=8;


             }


              if(sample_type & PERF_SAMPLE_ADDR)
               { // long long addr;
                   memcpy(&addr,&data[offset],sizeof(long long));
                 //    printf("ADDR:%llx ",addr);
                
                  offset+=8;

                }

                 
                  if(sample_type & PERF_SAMPLE_ID)
                  { memcpy(&id,&data[offset],sizeof(long long));
                    offset+=8;

                   }

                     if(sample_type & PERF_SAMPLE_STREAM_ID)
                    { memcpy(&stream_id,&data[offset],sizeof(long long));
                      offset+=8;

                     }

                           if(sample_type & PERF_SAMPLE_CPU)
                          { memcpy(&cpu,&data[offset],sizeof(int));
                            memcpy(&res,&data[offset+4],sizeof(int));
                            
                               offset+=8;

                           }

                         
                        if(sample_type & PERF_SAMPLE_PERIOD)
                        { memcpy(&period,&data[offset],sizeof(long long));
                         offset+=8;

                         }

                     /* if(sample_type & PERF_SAMPLE_READ)
                        { memcpy(&v,&data[offset],sizeof(struct read_format));
                         offset+=sizeof(struct read_format);

                         } */

                            
                       if(sample_type & PERF_SAMPLE_CALLCHAIN)
                        { memcpy(&nr,&data[offset],sizeof(long long));
                         offset+=8;
                         long long   ips[nr];    /* if PERF_SAMPLE_CALLCHAIN */
              
                           int i=0;
                           for(i=0;i<nr;i++)
                           { 
                             memcpy(&ips[i],&data[offset],sizeof(long long));
                             offset+=8;
                           }
                         }
                       
                        if(sample_type & PERF_SAMPLE_RAW)
                        { memcpy(&size_raw,&data[offset],sizeof(int));
                         offset+=4;
                         char   data[size_raw];     /* if PERF_SAMPLE_RAW */
              
                           int i=0;
                           for(i=0;i<size_raw;i++)
                           { 
                             memcpy(&data[i],&data[offset],sizeof(char));
                             offset+=1;
                           }
                         }
                                     
                      if(sample_type & PERF_SAMPLE_BRANCH_STACK)
                        { memcpy(&bnr,&data[offset],sizeof(long long));
                         offset+=8;
                          struct perf_branch_entry lbr[bnr];   /* if PERF_SAMPLE_BRANCH_STACK */
                           int i;

                           for(i=0;i<bnr;i++)
                           {
                             memcpy(&lbr[i],&data[offset],sizeof(struct perf_branch_entry));
                              offset+=sizeof(struct perf_branch_entry);
                            
                           }

                        }         
                      
                     if(sample_type & PERF_SAMPLE_REGS_USER)
                        { memcpy(&abi,&data[offset],sizeof(long long));
                         offset+=8;
                          //long long  regs[weight(mask)];
                          /*
                           not complete
                          */

                         }

                     if(sample_type & PERF_SAMPLE_STACK_USER)
                        { memcpy(&size_stack,&data[offset],sizeof(long long));
                         offset+=8;
                           char data2[size_stack];
                           int i;
                           for(i=0;i<size_stack;i++)
                           {memcpy(&data2[i],&data[offset],sizeof(char));
                              offset+=1;
                            }
                            long long dyn_size;
                          memcpy(&dyn_size,&data[offset],sizeof(long long));
                            offset+=8;
                            

                        }  
    

               if(sample_type & PERF_SAMPLE_WEIGHT)
                  { 
                        memcpy(&weight,&data[offset],sizeof(long long));
                           offset+=8;
                   }

                 if(sample_type & PERF_SAMPLE_DATA_SRC)
                  {// char dataSource[100];
                   memcpy(&data_src,&data[offset],sizeof(long long));
                     printf("D_SRC :%llx \n",data_src);
                   offset+=8;
                        strcpy(dataSource,"");
                     findSource(&dataSource,data_src);
                  }

              
       printf("PID:%d \t TID:%d \t IP:0x%llx \t  ADDR:0x%llx \t CPU:%d \t PERIOD:%lld\t WEIGHT:%lld \t DATA_SRC:%s\n",pid,tid,ip,addr,cpu,period,weight,dataSource);

        count++;
      } //if close    

      loffset=loffset+event->size;
    }

    printf("Count %d\n",count);
  }
  
 control_page->data_tail=head;

  free(data);

 return 1;
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

/*
addr
    If PERF_SAMPLE_ADDR is enabled, then a 64-bit address is included. This is usually the address of a tracepoint, breakpoint, or software event; otherwise the value is 0
*/
  struct perf_event_attr  pe;
  struct perf_event_attr  pe2;

  
  memset(&pe,0,sizeof(struct perf_event_attr));
  pe.type=PERF_TYPE_RAW;
  pe.size=sizeof(struct perf_event_attr);
  pe.config=0x10d3;
  pe.sample_period=SAMPLE_PERIOD;
  pe.sample_type=PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_ADDR |PERF_SAMPLE_CPU|PERF_SAMPLE_PERIOD| PERF_SAMPLE_WEIGHT|  PERF_SAMPLE_DATA_SRC ;
  pe.freq=0; //no requency
  pe.mmap=1;
 // pe.mmap_data=1;
  
  pe.read_format = 1;
  pe.pinned = 1;
  pe.mmap = 1;
  pe.task = 1;
  pe.wakeup_events = 1;
  pe.precise_ip = 2;

  pe.task=1; 
  pe.exclude_kernel=1;
  pe.exclude_hv=1;
  pe.disabled=1;
 // pe.sample_id_all=1; 
  int fd;


    memset(&pe2,0,sizeof(struct perf_event_attr));
  pe2.type=PERF_TYPE_RAW;
  pe2.size=sizeof(struct perf_event_attr);
  pe2.config=0x10d3;
  pe2.sample_period=SAMPLE_PERIOD;
  pe2.sample_type=PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_ADDR |PERF_SAMPLE_CPU | PERF_SAMPLE_PERIOD | PERF_SAMPLE_WEIGHT | PERF_SAMPLE_DATA_SRC ;
  pe2.freq=0; //no requency
  pe2.mmap=1;
 // pe2.mmap_data=1;
  
  pe2.read_format = 1;
  pe2.pinned = 1;
  pe2.mmap = 1;
  pe2.task = 1;
  pe2.wakeup_events = 1;
  pe2.precise_ip = 2;

  pe2.task=1; 
  pe2.exclude_kernel=1;
  pe2.exclude_hv=1;
  pe2.disabled=1;
 // pe2.sample_id_all=1; 
  int fd2;

 //sample on core 0

fd=perf_event_open(&pe,-1,0,-1,PERF_FLAG_FD_OUTPUT);
//   fd=perf_event_open(&pe,-1,0,-1,0);
  fd2=perf_event_open(&pe2,-1,1,-1,PERF_FLAG_FD_OUTPUT);
 //  fd2=perf_event_open(&pe2,-1,1,-1,0);
  if( fd==-1|| fd2==-1)
   {
      printf("Error openong leadder %llx\n",pe.config);
      exit(EXIT_FAILURE);
   }


 map_page1=mmap(NULL,(PAGE_COUNT + 1) * PAGE_SIZE,PROT_WRITE | PROT_READ,MAP_SHARED,fd,0);
 map_page2=mmap(NULL,(PAGE_COUNT + 1) * PAGE_SIZE,PROT_WRITE | PROT_READ,MAP_SHARED,fd2,0);

  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd2, PERF_EVENT_IOC_RESET, 0);
  
   ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
   ioctl(fd2, PERF_EVENT_IOC_ENABLE, 0);

  //coherence event threshold 572,000 events per second
  ////so for 0.1 second it is 57,200 events
  ////if sampling period is sample every 100 events then we should get a total of 572 samples
  
 
   nanosleep(&tim,NULL);
 
   //nanosec

  ioctl(fd, PERF_EVENT_IOC_REFRESH, 0);
  ioctl(fd2, PERF_EVENT_IOC_REFRESH, 0);

  read(fd, &count, sizeof(long long));
  read(fd2, &count2, sizeof(long long));

  printf("Core 0 %lld  HITM events occurred.\n", count);
  printf("Core 1 %lld  HITM events occurred.\n", count2);

 // printf("Samples collected on core 0\n");
   int p=perf_mmap_read(map_page1,(PAGE_COUNT + 1) ,prev_head,pe.sample_type,pe.read_format);
  printf("Samples collected on core 1\n");
 int  p2=perf_mmap_read(map_page2,(PAGE_COUNT+1),prev_head2,pe2.sample_type,pe2.read_format);
  
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
