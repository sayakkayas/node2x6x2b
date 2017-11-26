#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define _STDC_FORMAT_MACROS
#include <inttypes.h>
#include <pthread.h>
#include <time.h>
#include<sys/syscall.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <errno.h>
#define rmb() asm volatile("lfence":::"memory")
#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define mb() asm volatile("mfence":::"memory")
#define ITER 70000000
#include <assert.h>
#include <errno.h>
#include <stdint.h>

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

int read_pagemap(char * path_buf, unsigned long virt_addr);



static int* global_var;

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
                          if(CPU_ISSET(core_id,&cpuset[core_id]))
                         printf("Process %d to core %d\n",tid,core_id);
                  }
                  else
                  {
                          printf("Failed to pin thread %u on core %d\n",tid,core_id);
                   }
}
//


int read_pagemap(char * path_buf, unsigned long virt_addr)
{
   printf("Big endian? %d\n", is_bigendian());
 
  f = fopen(path_buf, "rb");

   if(!f)
    {
      printf("Error! Cannot open %s\n", path_buf);
      return -1;
   }
   
   //Shifting by virt-addr-offset number of bytes
   //   //and multiplying by the size of an address (the size of an entry in pagemap file)
     file_offset = virt_addr / getpagesize() * PAGEMAP_ENTRY;
     printf("Vaddr: 0x%lx, Page_size: %d, Entry_size: %d\n", virt_addr, getpagesize(), PAGEMAP_ENTRY);
     printf("Reading %s at 0x%llx\n", path_buf, (unsigned long long) file_offset);
     status = fseek(f, file_offset, SEEK_SET);
 
          if(status)
          {
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
          printf("[%d]0x%x ", i, c);
        }
  
          for(i=0; i < PAGEMAP_ENTRY; i++)
          {
              printf("%d ",c_buf[i]);
            read_val = (read_val << 8) + c_buf[i];
          }
           printf("\n");
         printf("Result: 0x%llx\n", (unsigned long long) read_val);
                                            //if(GET_BIT(read_val, 63))
            if(GET_BIT(read_val, 63))
             printf("PFN: 0x%llx\n",(unsigned long long) GET_PFN(read_val));
            else
            printf("Page not present\n");
           if(GET_BIT(read_val, 62))
              printf("Page swapped\n");
                                                                                                                                                         fclose(f);
                                                                                                                                                      return 0;
 }

int main()
{
  global_var=mmap(NULL,sizeof((*global_var)*16),PROT_READ |PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS  ,-1,0);

   global_var[0]=0;

  if(fork()==0)
  {  //child 1
      #ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
    #else
    #error "SYS_gettid unavailable on this system"  
    #endif

     //pin producer to core 0  
     pin_this_thread_to_core(0,tid);
     printf("Child 1 global_var[0]:%p\n",&global_var[0]);
   //  pid=getpid();
   // virt_addr = &global_var[0];
  // if(pid!=-1)
    //  sprintf(path_buf, "/proc/%u/pagemap", pid);
   
//   read_pagemap(path_buf, virt_addr);
    printf("Child 1 start\n");
          
    int c=0;
     while(ITER > c)  
       {
               if(global_var[0]%2==0)
                {
                  (global_var[0])++;

                  c++;
                   }
         
         
 
        }



    exit(0);
  }
  else
  { //parent


         if(fork()==0)
         {//child 2
           
                  #ifdef SYS_gettid
                   pid_t tid = syscall(SYS_gettid);
                  #else
                   #error "SYS_gettid unavailable on this system"  
                    #endif

                  //pin producer to core 1  
                  pin_this_thread_to_core(1,tid);
                   // pid=getpid();
		  // virt_addr = &global_var[0];
  		// if(pid!=-1)
     		//	 sprintf(path_buf, "/proc/%u/pagemap", pid);
   
  		// read_pagemap(path_buf, virt_addr);

               // char PID[50];    
              // sprintf(PID,"pmap %d",getpid());
                // char call[50];
                  //   system(PID);  
                  //
             printf("Child 2 global_var[0]:%p\n",&global_var[0]);
                  printf("Child 2 start\n");
        
            int c=0;
 	         while(ITER > c)
      		 {
             		  if(global_var[0]%2==1)
               		 {
                		  (global_var[0])++;
                            c++;
               		 }

          		 

       		 }



           exit(0);
          }
    
  
 
   }

int i;
for(i=0;i<2;i++)
wait(NULL);

printf("%d\n",*global_var);
munmap(global_var,sizeof(*global_var)*16);


return 0;


}
