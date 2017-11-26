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


//virtual to physical

typedef struct 
{
    uint64_t pfn : 54;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
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
    while (nread < sizeof(data)) {
        ret = pread(pagemap_fd, &data, sizeof(data),
                (vaddr / sysconf(_SC_PAGE_SIZE)) * sizeof(data) + nread);
        nread += ret;
        if (ret <= 0) {
            return 1;
        }
    }
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

    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap", (uintmax_t)pid);
    pagemap_fd = open(pagemap_file, O_RDONLY);
    if (pagemap_fd < 0) 
    {
        return 1;
    }
    PagemapEntry entry;
    if (pagemap_get_entry(&entry, pagemap_fd, vaddr)) 
    {
        return 1;
    }
    close(pagemap_fd);
    *paddr = (entry.pfn * sysconf(_SC_PAGE_SIZE)) + (vaddr % sysconf(_SC_PAGE_SIZE));
    return 0;
}


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


int main()
{

key_t shm_key = 6166529;
 
  int  shm_id;        /* shared memory ID      */
 
 shm_id = shmget(shm_key, 4*sizeof(int), IPC_CREAT | 0666);

if(shm_id < 0)
{
     printf("shmget error\n");
     exit(1);
}



  int* shm_at;
     shm_at = (int *)shmat(shm_id, NULL, 0);
     shm_at[0]=5;
    //mem=7;
    pid_t pid=getpid();
     printf("Virtual address in %d is %p\n",pid,shm_at);

      uintptr_t  paddr = 0;
      if (virt_to_phys_user(&paddr, pid, shm_at))
       {
        fprintf(stderr, "error: virt_to_phys_user\n");
        return EXIT_FAILURE;
        }
         printf("PID %d Physical address 0x%jx\n",pid, (uintmax_t)paddr);


         printf("\n");
    //readPage map
     char path[100]; char PID[10];
 
    strcpy(path,"/proc/");
    sprintf(PID,"%d",getpid());
    strcat(path,PID);
    strcat(path,"/pagemap");

    read_pagemap(pid,path,shm_at);  

    printf("\n");
    


  return 0;
}
