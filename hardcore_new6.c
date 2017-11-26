/*Compile-gcc perf2.c -o perf_EXC -w
 * Run-sudo ./perf_EXC //sudo privilege for sysmtem wide sampling
 */

/*Events on Intel XEon E5-2660
 * r10d3-Retired load uops whose data sources were remote HITM  //helps to understand cross sockect traffic as to cache line being constantly transfferd in M state
*/

//PHYSICAL address part
#define _XOPEN_SOURCE 700
#include <fcntl.h> /* open */
#include <stdint.h> /* uint64_t  */
#include <stdio.h> /* printf */
#include <stdlib.h> /* size_t */
#include <unistd.h> /* pread, sysconf */
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>

//read page map
#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

int i, c, pid, status;
unsigned long virt_addr; 
uint64_t read_val, file_offset;
char path_buf [0x100] = {};
FILE * f;
char *end;




//virtual to physical(this is being used)

typedef struct 
{
   //54 bits represent page frame number
    uint64_t pfn : 54;
   //status of the page soft or dirty
    unsigned int soft_dirty : 1;
   //whether it's a page file or not
    unsigned int file_page : 1;
   //whether the page has been swapped or not
    unsigned int swapped : 1;
    //whether teh page is present or not
    unsigned int present : 1;
} PagemapEntry;

/* Parse the pagemap entry for the given virtual address.
 *
 * @param[out] entry      the parsed entry
 * @param[in]  pagemap_fd file descriptor to an open /proc/pid/pagemap file
 * @param[in]  vaddr      virtual address to get entry for
 * @return 0 for success, 1 for failure
 */
int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr)
{
    size_t nread;
    ssize_t ret;
    uint64_t data;

    nread = 0;
   //loop to read data from the file
    while (nread < sizeof(data))
     {   //read chunks of 64 bits
        ret = pread(pagemap_fd, &data, sizeof(data),(vaddr / sysconf(_SC_PAGE_SIZE)) * sizeof(data) + nread);
      
         nread += ret;
        if (ret <= 0)
         {
            return 1;
        }

    } //while close
 
    //get page frame number  
     entry->pfn = data & (((uint64_t)1 << 54) - 1);
    entry->soft_dirty = (data >> 54) & 1;
    entry->file_page = (data >> 61) & 1;
    entry->swapped = (data >> 62) & 1;
    entry->present = (data >> 63) & 1;
    return 0;
}

/* Convert the given virtual address to physical using /proc/PID/pagemap.
 *
 * @param[out] paddr physical address
 * @param[in]  pid   process to convert for
 * @param[in] vaddr virtual address to get entry for
 * @return 0 for success, 1 for failure
 */
unsigned long virt_to_phys_user( pid_t pid, uintptr_t vaddr)
{
    char pagemap_file[BUFSIZ];
    int pagemap_fd;
    //get path of pagemap
    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap", (uintmax_t)pid);

    pagemap_fd = open(pagemap_file, O_RDONLY); //open pagemap of the desired process

   //if fail toopen pagemap 
   if (pagemap_fd < 0) 
    {
        return 1;
    }


    PagemapEntry entry;
    //get pagemap entry in the structure
    if (pagemap_get_entry(&entry, pagemap_fd, vaddr)) 
    {
        return 1;
    }
     //close pagemap file
    close(pagemap_fd);

    unsigned long paddr;
    //physical address is computed as PFN*PAGE_SIZE+OFFSET(vaddr% PAGESIZE)
    paddr = (entry.pfn * sysconf(_SC_PAGE_SIZE)) + (vaddr % sysconf(_SC_PAGE_SIZE));
    return paddr;
}

/*
int read_pagemap(pid_t pid,char * path_buf, unsigned long virt_addr)
{
   //printf("Big endian? %d\n", is_bigendian());
   f = fopen(path_buf, "rb");
   if(!f){
      printf("Error! Cannot open %s\n", path_buf);
      return -1;
   }
   
   //Shifting by virt-addr-offset number of bytes
   //and multiplying by the size of an address (the size of an entry in pagemap file)
   file_offset = virt_addr / getpagesize() * PAGEMAP_ENTRY;
   printf("PID:%d Vaddr: 0x%lx, Page_size: %d, Entry_size: %d\n",pid, virt_addr, getpagesize(), PAGEMAP_ENTRY);
   printf("PID:%d Reading %s at 0x%llx\n",pid, path_buf, (unsigned long long) file_offset);
   status = fseek(f, file_offset, SEEK_SET);
   if(status){
      perror("Failed to do fseek!");
      return -1;
   }
   errno = 0;
   read_val = 0;
   unsigned char c_buf[PAGEMAP_ENTRY];
   for(i=0; i < PAGEMAP_ENTRY; i++)
   {
      c = getc(f);
      if(c==EOF)
      {
         printf("\nReached end of the file\n");
         return 0;
      }
      if(is_bigendian())
           c_buf[i] = c;
      else
           c_buf[PAGEMAP_ENTRY - i - 1] = c;
      //printf("[%d]0x%x ", i, c);
   }

   for(i=0; i < PAGEMAP_ENTRY; i++)
   {
      //printf("%d ",c_buf[i]);
      read_val = (read_val << 8) + c_buf[i];
   }

   printf("\n");
   printf("PID:%d Result: 0x%llx\n",pid, (unsigned long long) read_val);
   //if(GET_BIT(read_val, 63))
   if(GET_BIT(read_val, 63))
      printf("PID:%d PFN: 0x%llx\n",pid,(unsigned long long) GET_PFN(read_val));
   else
      printf("Page not present\n");
   if(GET_BIT(read_val, 62))
      printf("Page swapped\n");
   fclose(f);
   return 0;
}

*/


//add virtual to physical translation and then populate filter with physical address or PFN and check results
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
#include <stdbool.h>
#include <string.h>

#define rmb() asm volatile("lfence":::"memory")

#define PAGE_COUNT 64
#define PAGE_SIZE getpagesize()
#define BYTE_COUNT (PAGE_COUNT * PAGE_SIZE)
#define SAMPLE_PERIOD 10000
#define CORES 10
#define THRESHOLD_VAL 2860000   //should be dependent on the time interval
/*Threshold value defined in [1] is 2.6 x10^-4 coherence events per cycle
 * CPU running on is 2.20 GHz
 * 1 cycle is 1/2.20 * 10^-9 secs
 * As sampling rate is 5 seconds ,5 seconds have,
 * 5/(1/2.20*10^-9) cycles,
 * each cycle should have 2.6 * 10^-4 coherence events 
 * so 5 seconds should have 5/(1/2.20*10^-9) * 2.6 * 10^-4 coherence events which should be our threshold
 *=2,860,000
 *
 */



#define UNIQUE 100
#define THREAD_MAX 100
#define M 1000

struct thread_info
{
 pid_t pid;
 pid_t tid;
 int core;
 long long mem_addr[UNIQUE];
 int freq[UNIQUE];
 int mem_addr_index;
//bloom filter
 bool filter[M];
 int filter_freq[M];   


 //newly added for physical addresses
 unsigned long phy_mem_addr[UNIQUE];
 int phy_freq[UNIQUE];
 int phy_mem_addr_index;
 //bloom filter
 bool phy_filter[M];
 int phy_filter_freq[M]; 
};

struct thread_info threads[THREAD_MAX];
int thread_index=0;


void initialize()
{

    int i=0;
     int j,k;

    for(i=0;i<THREAD_MAX;i++)
     {
        
         for(j=0;j<M;j++)
          {
              threads[i].filter[j]=false;
              threads[i].phy_filter[j]=false;
              threads[i].filter_freq[j]=0;
              threads[i].phy_filter_freq[j]=0;


          }

     }


}

bool resFilter[M];
//Pairs
struct Pair
{
int i;
int j;
};

struct Pair P[100];
int pair_index=0;





//perf event parameters and variables
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


int count_events()
{

	    //Counting event remote HITM 
      struct perf_event_attr pe[CORES];   //for event count on cores
      long long count[CORES];            //counts and stores  events on cores 
      int fd[CORES];
 int ret=0; //return value

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
        if(count[i]>THRESHOLD_VAL)
        {  printf("Coherence activity greater than threshold value at core %d\n",i);
                 
             ret=1;
          }

      close(fd[i]);
    }    
    
    return ret;

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

              
     // printf("PID:%d \t TID:%d \t IP:0x%llx \t  ADDR:0x%llx \t CPU:%d \t PERIOD:%lld\t \n",pid,tid,ip,addr,cpu,period);
        storeSample(pid,tid,addr,cpu);

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

/*
void populatePhysical()
{

  int i; int j;
    for(i=0;i<thread_index;i++)
    {
       //printf("PID:%d TID:%d CPU:%d\n",threads[i].pid,threads[i].tid,threads[i].core);

        for(j=0;j<threads[i].mem_addr_index;j++)
        {

          virt_to_phys_user(&(threads[i].phy_mem_addr[j]), threads[i].pid,threads[i].mem_addr[j]);
             threads[i].phy_freq[j]=threads[i].freq[j];           

         }
    }      

}
*/

void displaySample()
{

 int i; int j;
    for(i=0;i<thread_index;i++)
    {
       printf("PID:%d TID:%d CPU:%d\n",threads[i].pid,threads[i].tid,threads[i].core);

        for(j=0;j<threads[i].mem_addr_index;j++)
        {
 printf("Virt Memory:%p  Physical Memory:%p VFreq:%d PFreq:%d\n",threads[i].mem_addr[j],threads[i].phy_mem_addr[j],threads[i].freq[j],threads[i].phy_freq[j]);
     int vtotal=TotalFrequency(threads[i].filter,threads[i].filter_freq);
     int ptotal=TotalFrequency(threads[i].phy_filter,threads[i].phy_filter_freq);

        printf("VFF:%d PFF:%d\n",vtotal,ptotal);
        }

          /*
         printf("Filter :");
        for(j=0;j<M;j++)
         printf("%d",threads[i].filter[j]);
           */

       printf("\n");

     } 
   
  

}



int hash(long long addr)
{

char key[256];
sprintf(key,"%lld",addr);
 uint32_t len=(uint32_t)strlen(key);
 uint32_t seed=0;


  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;
  uint32_t r1 = 15;
  uint32_t r2 = 13;
  uint32_t m = 5;
  uint32_t n = 0xe6546b64;
  uint32_t h = 0;
  uint32_t k = 0;
  uint8_t *d = (uint8_t *) key; // 32 bit extract from `key'
  const uint32_t *chunks = NULL;
  const uint8_t *tail = NULL; // tail - last 8 bytes
  int i = 0;
  int l = len / 4; // chunk length

  h = seed;
	
  chunks = (const uint32_t *) (d + l * 4); // body
  tail = (const uint8_t *) (d + l * 4); // last 8 byte chunk of `key'

//for each 4 byte chunk of `key'
   for (i = -l; i != 0; ++i) {
  //       // next 4 byte chunk of `key'
            k = chunks[i];
  //
  //               // encode next 4 byte chunk of `key'
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
  //
  //                               // append to hash
           h ^= k;
           h = (h << r2) | (h >> (32 - r2));
           h = h * m + n;
       }
 
         k = 0;
         // remainder
         switch (len & 3) { // `len % 4'
          case 3: k ^= (tail[2] << 16);
           case 2: k ^= (tail[1] << 8);
           case 1:
             k ^= tail[0];
             k *= c1;
            k = (k << r1) | (k >> (32 - r1));
           k *= c2;
            h ^= k;
                      }
                  h ^= len;
             h ^= (h >> 16);
              h *= 0x85ebca6b;
                 h ^= (h >> 13);
               h *= 0xc2b2ae35;
              h ^= (h >> 16);
                return (h%M);
  //
  //
  //
           }
  
  /*
 int hash(long long addr)
{
  
  long long g;
  g=(addr*(addr-1)*5)+3;
  g=g%M;

 // printf("%llx %lld\n",addr,g);
   return g;
}*/

void storeSample(int pid,int tid,long long addr,int cpu)
{

    /*struct thread_info
    {
      pid_t processID;
      pid_t threadID;
      int core;
      long long mem_addr[UNIQUE];
      int freq[UNIQUE];
      int mem_addr_index;     
    };

    struct thread_info threads[THREAD_MAX];
    int thread_index=0;
     */

  int i;int flag=0;
    for(i=0;i<thread_index;i++)
    {
        if(threads[i].tid==tid)
          {
            flag=1;
             break;
          }
    }//for close

    //if flag==0,then not found then

    if(flag==0)
    {
        threads[thread_index].pid=pid;
        threads[thread_index].tid=tid;
        threads[thread_index].core=cpu;
        threads[thread_index].mem_addr_index=0;
        threads[thread_index].mem_addr[threads[thread_index].mem_addr_index]=addr;
        threads[thread_index].freq[threads[thread_index].mem_addr_index]=1;
        threads[thread_index].filter[hash(addr)]=true;
        threads[thread_index].filter_freq[hash(addr)]+=1;
        
        //new addition
        unsigned long phy_addr=virt_to_phys_user(pid,addr);
        threads[thread_index].phy_mem_addr_index=0;
        threads[thread_index].phy_mem_addr[threads[thread_index].phy_mem_addr_index]=phy_addr;
        threads[thread_index].phy_freq[threads[thread_index].phy_mem_addr_index]=1;
        threads[thread_index].phy_filter[hash(phy_addr)]=true;
        threads[thread_index].phy_filter_freq[hash(phy_addr)]+=1;
            
        threads[thread_index].mem_addr_index++;
        threads[thread_index].phy_mem_addr_index++;
        
        thread_index++;

    }//if close
    else
    {
      threads[i].mem_addr[threads[i].mem_addr_index]=addr;
      //new addition
      unsigned long phy_addr=virt_to_phys_user(pid,addr);
        threads[i].phy_mem_addr[threads[i].phy_mem_addr_index]=phy_addr;
      
         int j;
         int f=0;
          for(j=0;j<threads[i].mem_addr_index;j++)
           {
              if(addr==threads[i].mem_addr[j])
               {threads[i].freq[j]=threads[i].freq[j]+1;
                  f=1;
                  break;
                }
           }  
 
             //f==0 new address entry
          if(f==0)
          {
             threads[i].mem_addr[threads[i].mem_addr_index]=addr;
             threads[i].filter[hash(addr)]=true;
             threads[i].filter_freq[hash(addr)]+=1;
            //printf("INC Filter freq\n");
              threads[i].freq[threads[i].mem_addr_index]=1;
              threads[i].mem_addr_index++;
        

          }

           int f2=0;

           for(j=0;j<threads[i].phy_mem_addr_index;j++)
           {
              if(phy_addr==threads[i].phy_mem_addr[j])
               {threads[i].phy_freq[j]= threads[i].phy_freq[j]+1;
                  f2=1;
                  break;
                }
           }  
 
             //f==0 new address entry
          if(f2==0)
          {
             threads[i].phy_mem_addr[threads[i].phy_mem_addr_index]=phy_addr;
             threads[i].phy_filter[hash(phy_addr)]=true;
             threads[i].phy_filter_freq[hash(phy_addr)]+=1;
            //printf("INC phy filter freq\n");
              threads[i].phy_freq[threads[i].phy_mem_addr_index]=1;
              threads[i].phy_mem_addr_index++;
        

          }



    }//else close


}//function close

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
  pe[i].config=0x10d3;
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
  // sleep(2);
   //nanosec

for(i=0;i<CORES;i++)
 { ioctl(fd[i], PERF_EVENT_IOC_DISABLE, 0);
 }

 /*
  for(i=0;i<CORES;i++)
  read(fd[i], &countEVENT[i], sizeof(long long));
  
   for(i=0;i<CORES;i++)
   printf("Core %d %lld  HITM events occurred.\n",i, countEVENT[i]);
  */
 
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

/*
struct Bfilter
{
         bool taken;
        pid_t tid;
    bool filter[M];
    int frequency[M];

};

*/


int numbBitsSet(bool * filter)
{
	int i;
	int count=0;
	 for(i=0;i<M;i++)
	 { 
	 	 if(filter[i]==true)
             count++;
	 }

  return count;
}

int IntersectFilter(bool* filter1,bool* filter2,int* freq1,int* freq2,int* count_freq1,int* count_freq2)
{
    int i;
	int count=0;

	 for(i=0;i<M;i++)
	 {
	    if(filter1[i]==true && filter2[i]==true )      
	     { count++;
           (*count_freq1)+=freq1[i];
           (*count_freq2)+=freq2[i];
           
        } 
     }

   return count;
}


int TotalFrequency(bool* filter,int* freq)
{

 int i;
  int TotalFreq=0;
   for(i=0;i<M;i++)
   { 
     if(filter[i]==true)
            TotalFreq=TotalFreq+freq[i];
   }

return TotalFreq;

}

/*Probability of false set overlap for unpartiotioned bloom filter intersection is:-
P=(1-(1-1/m)^(k^2*|S1|*|S2|))

k=1
M=1000
|S1|=31,|S2|=31

P=1-0.36
P=0.64
This is bad we need to reduce this to 0.2
*/

int checkPair(int i,int j)
{

     int index=0;
      for(index=0;index<pair_index;i++)
      {
          if( (P[index].i==i && P[index].j==j) || (P[index].i==j && P[index].j==i) )
             return 0;
      }


     P[pair_index].i=i;
     P[pair_index].j=j;
     
  pair_index++;

  return 1;
}

void checkIntersection()
{
	int i,j;

	 for(i=0;i<thread_index;i++)
	 {
        for(j=0;j<thread_index;j++)
         {
         	if(i!=j)
         	{ 

         	    //if(checkPair(i,j))
         		{	
         			int t1=numbBitsSet(threads[i].filter);
         			int t2=numbBitsSet(threads[j].filter);
            		int freqt1=TotalFrequency(threads[i].filter,threads[i].filter_freq);
            		int freqt2=TotalFrequency(threads[j].filter,threads[j].filter_freq);
              		int freq_t1=0,freq_t2=0;
    int inter=IntersectFilter(threads[i].filter,threads[j].filter,threads[i].filter_freq,threads[j].filter_freq,&freq_t1,&freq_t2);
printf("%d,%d,%d,%d,%d,%d\n",t1,t2,freqt1,freqt2,freq_t1,freq_t2);   
 printf("Thread %d has %lf in intersection with Thread %d\n",threads[i].tid,(((double)inter)/((double)t1))*100,threads[j].tid);
    printf("Thread %d has %lf in intersection Thread %d\n",threads[j].tid,(((double)inter)/((double)t2))*100,threads[i].tid);
   printf("Thread %d has %lf percent frequency in intersection with Thread %d\n",threads[i].tid,(((double)freq_t1)/((double)freqt1))*100,threads[j].tid);
   printf("Thread %d has %lf percent frequency in intersection with Thread %d\n",threads[j].tid,(((double)freq_t2)/((double)freqt2))*100,threads[i].tid);
             
         		 	printf("\n");
         	
              		t1=numbBitsSet(threads[i].phy_filter);
       	 		t2=numbBitsSet(threads[j].phy_filter);
            		freqt1=TotalFrequency(threads[i].phy_filter,threads[i].phy_filter_freq);
            		freqt2=TotalFrequency(threads[j].phy_filter,threads[j].phy_filter_freq);
            		freq_t1=0,freq_t2=0;
   inter=IntersectFilter(threads[i].phy_filter,threads[j].phy_filter,threads[i].phy_filter_freq,threads[j].phy_filter_freq,&freq_t1,&freq_t2);
               printf("%d,%d,%d,%d,%d,%d\n",t1,t2,freqt1,freqt2,freq_t1,freq_t2);
    printf("Physical Thread %d has %lf in intersection with Thread %d\n",threads[i].tid,(((double)inter)/((double)t1))*100,threads[j].tid);
    printf("Physical Thread %d has %lf in intersection with Thread %d\n",threads[j].tid,(((double)inter)/((double)t2))*100,threads[i].tid);
    printf("Physical Thread %d has %lf percent frequency in intersection with Threads %d\n",threads[i].tid,(((double)freq_t1)/((double)freqt1))*100,threads[j].tid);
    printf("Physical Thread %d has %lf percent frequency in intersection with Threads %d\n",threads[j].tid,(((double)freq_t2)/((double)freqt2))*100,threads[i].tid);
             
         		 	printf("\n");
         	    }//if close


         	}
         }      

	 }

}

int main()
{

 initialize();
printf("Counting remote HIT modified on cores for 5 second\n");
 if(count_events())
 {
   printf("Event count greater than threshold ,hence sampling for 0.1 seconds on all cores\n");
    sample(); 
    printf("\n");
   // populatePhysical();
    displaySample();
    checkIntersection();
    //populateBF();
  }   
return 0;
}
