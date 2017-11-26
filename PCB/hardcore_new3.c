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
#define CORES 2
struct perf_event_mmap_page *map_page[CORES];


long long prev_head[CORES];// = 0;
unsigned char *data[CORES];// = NULL;

//long long prev_head2 = 0;
//unsigned char *data2 = NULL;


static long perf_event_open(struct perf_event_attr * hw_event,pid_t pid,int cpu,int group_fd,unsigned long flags)
{
  int ret;
  ret=syscall(__NR_perf_event_open,hw_event,pid,cpu,group_fd,flags);

  return ret;

}


void count_events()
{

	//Counting event remote HITM 
     struct perf_event_attr pe[CORES];   //for event count on cores
    long long count[CORES];            //counts and stores  events on cores 
      int fd[CORES];
 

int i;
for(i=0;i<CORES;i++)
 {
  memset(&pe[i],0,sizeof(struct perf_event_attr));   
  pe[i].type=PERF_TYPE_RAW;
 pe[i].size=sizeof(struct perf_event_attr);
 pe[i].config=0x10d3;
 pe[i].disabled=1;
 pe[i].exclude_kernel=1;
 pe[i].exclude_hv=1;

 
 fd[i]=perf_event_open(&pe[i],-1,i,-1,0);

    if(fd[i]==-1  )
    { 
       printf("Error opening leader %llx \n",pe[i].config);
      exit(EXIT_FAILURE);
    }

 } 

 for(i=0;i<CORES;i++)
 {


   ioctl(fd[i],PERF_EVENT_IOC_RESET,0);
   ioctl(fd[i],PERF_EVENT_IOC_ENABLE,0);
 } 
    //system wide for two seconds
     sleep(5);
   // nanosleep(&tim,NULL);
 
 for(i=0;i<CORES;i++)
   ioctl(fd[i],PERF_EVENT_IOC_DISABLE,0);
  
    
    for(i=0;i<CORES;i++)
     read(fd[i],&count[i],sizeof(long long));
     
    
   for(i=0;i<CORES;i++)
    { 
       printf("EVENT remote cache HITM on core %d : %lld\n",i,count[i]);
    

      close(fd[i]);
    }    
    

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
                  //   printf("D_SRC :%llx \n",data_src);
                   offset+=8;
                        strcpy(dataSource,"");
                     findSource(&dataSource,data_src);
                  }

              
       printf("PID:%d \t TID:%d \t IP:0x%llx \t  ADDR:0x%llx \t CPU:%d \t PERIOD:%lld\t WEIGHT:%lld \t DATA_SRC:%s\n",pid,tid,ip,addr,cpu,period,weight,dataSource);

        count++;
      } //if close    

      loffset=loffset+event->size;
    } //while close

    printf("Count %d\n",count);
  }//if close
  
 control_page->data_tail=head;

  free(data);

 return 1;
}


void sample()
{

	//time for sampling set up

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
/*
D1H 20H MEM_LOAD_UOPS_RETIRED.HITM  Counts load uops retired where the cache line containing the data was Precise Event
in the modified state of another core or modules cache (HITM). More specifically, this means that when the load address was checked by
other caching agents (typically another processor) in the system, one of those caching agents indicated that they had a dirty copy of the
data. Loads that obtain a HITM response incur greater latency than most that is typical for a load. In addition, since HITM indicates that
some other processor had this data in its cache, it implies that the data was shared between processors, or potentially was a lock or
semaphore value. This event is useful for locating sharing, false sharing, and contended locks.*/
 struct perf_event_attr pe[CORES];   //for event count on cores
    long long countEVENT[CORES];            //counts and stores  events on cores 
      int fd[CORES];

int i;
 for(i=0;i<CORES;i++)
 countEVENT[i]=0; 


for(i=0;i<CORES;i++)
 {
  memset(&pe[i],0,sizeof(struct perf_event_attr));   
  pe[i].type=PERF_TYPE_RAW;
  pe[i].size=sizeof(struct perf_event_attr);
  pe[i].config=0x08d1;
  pe[i].sample_period=SAMPLE_PERIOD;
  pe[i].sample_type=PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_ADDR |PERF_SAMPLE_CPU|PERF_SAMPLE_PERIOD| PERF_SAMPLE_WEIGHT|  PERF_SAMPLE_DATA_SRC ;
  pe[i].freq=0; //no requency
  pe[i].mmap=1;
 // pe.mmap_data=1;
  
  pe[i].read_format = 1;
  pe[i].pinned = 1;
  pe[i].mmap = 1;
  pe[i].task = 1;
  pe[i].wakeup_events = 1;
  pe[i].precise_ip = 2;
  pe[i].exclude_kernel=1;
  pe[i].exclude_hv=1;
  pe[i].disabled=1;
 
 fd[i]=perf_event_open(&pe[i],-1,i,-1,0);

    if(fd[i]==-1  )
    { 
       printf("Error opening leader %llx \n",pe[i].config);
      exit(EXIT_FAILURE);
    }

 // map_page[i]=mmap(NULL,(PAGE_COUNT + 1) * PAGE_SIZE,PROT_WRITE | PROT_READ,MAP_SHARED,fd[i],0);
 

 }

  for(i=0;i<CORES;i++)
   map_page[i]=mmap(NULL,(PAGE_COUNT+1)*PAGE_SIZE,PROT_WRITE |PROT_READ,MAP_SHARED,fd[i],0); 

 for(i=0;i<CORES;i++)
 {


   ioctl(fd[i],PERF_EVENT_IOC_RESET,0);
   ioctl(fd[i],PERF_EVENT_IOC_ENABLE,0);
 } 

    //coherence event threshold 572,000 events per second
  ////so for 0.1 second it is 57,200 events
  ////if sampling period is sample every 100 events then we should get a total of 572 samples
  
 
   nanosleep(&tim,NULL);
 
   //nanosec

for(i=0;i<CORES;i++)
 { ioctl(fd[i], PERF_EVENT_IOC_DISABLE, 0);
 }

  for(i=0;i<CORES;i++)
  read(fd[i], &countEVENT[i], sizeof(long long));
  
   for(i=0;i<CORES;i++)
   printf("Core %d %lld  HITM events occurred.\n",i, countEVENT[i]);
  
 
  for(i=0;i<CORES;i++)
   {
    printf("Samples collected on core %d\n",i);
     prev_head[i]=0;
     data[i]=NULL;
    int p=perf_mmap_read(map_page[i],(PAGE_COUNT + 1) ,prev_head[i],pe[i].sample_type,pe[i].read_format);
   }

   for(i=0;i<CORES;i++)
   { munmap(map_page[i], (PAGE_COUNT + 1) * PAGE_SIZE);
    close(fd[i]);
    }


}

int main()
{

// count_events();
// sleep(1);
  sample(); 
return 0;
}
