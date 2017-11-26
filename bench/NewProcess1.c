#define _GNU_SOURCE
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
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include<sys/syscall.h>
//#include <stdatomic.h>
#include <errno.h>
#define handle_error_en(en, msg) \
           do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define ITER 100000000

void pin_this_thread_to_core(int core_id,pid_t tid) 
{
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
 
  if (core_id < 0 || core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset[4];
   CPU_ZERO(&cpuset[core_id]);
   CPU_SET(core_id, &cpuset[core_id]);

   pthread_t current_thread = pthread_self();    
   int no_err= pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset[core_id]);

   if(no_err==0)//success
    {
        if(CPU_ISSET(2,&cpuset[2]))
       printf("Thread %u assigned to core 2\n",tid);
       else
       {
       if(CPU_ISSET(3,&cpuset[3]))
       printf("Thread %u assigned to core 3\n",tid);
       }
     }
     else
      {
        printf("Failed to pin thread %u on core %d\n",tid,core_id);

       }

}


//virtual to physical

typedef struct 
{
  //54 bits represent the page frame number
    uint64_t pfn : 54;
    //status of the page soft or dirty
    unsigned int soft_dirty : 1;
    //file page bit
    unsigned int file_page : 1;
    //whether page is swapped
    unsigned int swapped : 1;
    //whether page is present
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
    {
         //read chunks of size 64 bits
        ret = pread(pagemap_fd, &data, sizeof(data),(vaddr / sysconf(_SC_PAGE_SIZE)) * sizeof(data) + nread);
        nread += ret;

        if (ret <= 0) 
        {
            return 1;
        }
    }
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
int virt_to_phys_user(uintptr_t *paddr, pid_t pid, uintptr_t vaddr)
{
    char pagemap_file[BUFSIZ];
    int pagemap_fd;

    //get path for pagemap
    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap", (uintmax_t)pid);
    pagemap_fd = open(pagemap_file, O_RDONLY);  //open pagemap file for the desired process
    
    //if fail to open pagemap
    if (pagemap_fd < 0) 
    {
        return 1;
    }

      //sucessfully opened pagemap
    PagemapEntry entry;

    //populate the entry
    if (pagemap_get_entry(&entry, pagemap_fd, vaddr)) 
    {
        return 1;
    }

   //close the file(pagemap)
    close(pagemap_fd);
     //physical address is computed by PFN*PAGESIZE+OFFSET(VADDR% PAGE_SIZE)         
    *paddr = (entry.pfn * sysconf(_SC_PAGE_SIZE)) + (vaddr % sysconf(_SC_PAGE_SIZE));
    
    return 0;
}


struct data
{
 // pthread_mutex_t lock;
 long  int ar[4][8];
 // pthread_mutex_t lock;

} ;


int main()
{

  pid_t pid=getpid();
  
  pin_this_thread_to_core(2,pid);


   /*Shared memory*/
   key_t shm_key = 6166537;
   int  shm_id;        /* shared memory ID      */
   // printf("%d\n",4*sizeof(int));
   
   shm_id = shmget(shm_key,sizeof(struct data), IPC_CREAT | 0666);

    if(shm_id < 0)
    {printf("shmget error\n");
     exit(1);
    }

    struct data* shm_at;
     shm_at = (struct data *)shmat(shm_id, NULL, 0);
    
     //synchronization variable
    /* key_t lock_key=6166533;
     int lock_id;
     lock_id=shmget(lock_key,sizeof(pthread_mutex_t),IPC_CREAT |0666);
     pthread_mutex_t* lock;
     lock=(pthread_mutex_t*)shmat(lock_id, NULL, 0);
    */

   //initialization
     shm_at->ar[0][0]=0;
     shm_at->ar[1][0]=0;
     shm_at->ar[2][0]=0;
     shm_at->ar[3][0]=0;
  // pthread_mutex_init(&(shm_at->lock),NULL);

    //mem=7;
    
     printf("PID %d Virtual address of shm_at->ar[0][0] is %p\n",pid,&(shm_at->ar[0][0]));
     printf("PID %d Virtual address of shm_at->ar[1][0] is %p\n",pid,&(shm_at->ar[1][0]));
     printf("PID %d Virtual address of shm_at->ar[2][0] is %p\n",pid,&(shm_at->ar[2][0]));
     printf("PID %d Virtual address of shm_at->ar[3][0] is %p\n",pid,&(shm_at->ar[3][0]));
   //   printf("PID %d Virtual address of shm_at->lock is %p\n",pid,&(shm_at->lock));
     

     printf("\n");

      uintptr_t  paddr = 0;
      if (virt_to_phys_user(&paddr, pid, &(shm_at->ar[0][0])))
       {
        fprintf(stderr, "error: virt_to_phys_user\n");
        return EXIT_FAILURE;
        }
         printf("PID %d shm_at->ar[0][0] Physical address 0x%jx\n",pid, (uintmax_t)paddr);

      paddr = 0;
      if (virt_to_phys_user(&paddr, pid, &(shm_at->ar[1][0])))
       {
        fprintf(stderr, "error: virt_to_phys_user\n");
        return EXIT_FAILURE;
        }
         printf("PID %d shm_at->ar[1][0] Physical address 0x%jx\n",pid, (uintmax_t)paddr);

        if (virt_to_phys_user(&paddr, pid, &(shm_at->ar[2][0])))
       {
        fprintf(stderr, "error: virt_to_phys_user\n");
        return EXIT_FAILURE;
        }
         printf("PID %d shm_at->ar[2][0] Physical address 0x%jx\n",pid, (uintmax_t)paddr);

      paddr = 0;
      if (virt_to_phys_user(&paddr, pid, &(shm_at->ar[3][0])))
       {
        fprintf(stderr, "error: virt_to_phys_user\n");
        return EXIT_FAILURE;
        }
         printf("PID %d shm_at->ar[3][0] Physical address 0x%jx\n",pid, (uintmax_t)paddr);
         printf("\n");

    /*
        paddr = 0;
      if (virt_to_phys_user(&paddr, pid, &(shm_at->lock)))
       {
        fprintf(stderr, "error: virt_to_phys_user\n");
        return EXIT_FAILURE;
        }
         printf("PID %d shm_at->lock Physical address 0x%jx\n",pid, (uintmax_t)paddr);
         printf("\n");
         */

     
        int count=0;

       
         while((shm_at->ar[0][0])<ITER)
         {

               if((shm_at->ar[0][0])%2==0)
               {
               //  pthread_mutex_lock(&(shm_at->lock));
                // (shm_at->ar[0])=(shm_at->ar[0])+1;
                 (shm_at->ar[1][0])=(shm_at->ar[1][0])+1;
                 (shm_at->ar[2][0])=(shm_at->ar[2][0])+1;
                 (shm_at->ar[3][0])=(shm_at->ar[3][0])+1;
                 (shm_at->ar[0][0])=(shm_at->ar[0][0])+1;
                 //  count++;
               // pthread_mutex_unlock(&(shm_at->lock));

                 // printf("PID %d Count %d\n",pid,count);
               }


         } //while close
     
    
      
      printf("PID %d Val shm_at->ar[0][0] %d\n",pid,shm_at->ar[0][0]);
      printf("PID %d Val shm_at->ar[1][0] %d\n",pid,shm_at->ar[1][0]);
      printf("PID %d Val shm_at->ar[2][0] %d\n",pid,shm_at->ar[2][0]);
      printf("PID %d Val shm_at->ar[3][0] %d\n",pid,shm_at->ar[3][0]);
    

   
  return 0;
}
