#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include<stdint.h>
#define PAGEMAP_LENGTH 8
#define PAGE_SHIFT 12


unsigned long get_page_frame_number_of_address(void *addr)
 {
   // Open the pagemap file for the current process
   FILE *pagemap = fopen("/proc/self/pagemap", "rb");

   // Seek to the page that the buffer is on
   unsigned long offset = (unsigned long)addr / getpagesize() * PAGEMAP_LENGTH;

   if(fseek(pagemap, (unsigned long)offset, SEEK_SET) != 0) 
   {
      fprintf(stderr, "Failed to seek pagemap to proper location\n");
      exit(1);
   }

   // The page frame number is in bits 0-54 so read the first 7 bytes and clear the 55th bit
   unsigned long page_frame_number = 0;
   fread(&page_frame_number, 1, PAGEMAP_LENGTH-1, pagemap);

   page_frame_number &= 0x7FFFFFFFFFFFFF;

   fclose(pagemap);

   return page_frame_number;
}


void VtoP(void* buffer)
{
  //void *buffer =&count;

   // Get the page frame the buffer is on
   unsigned int page_frame_number = get_page_frame_number_of_address(buffer);
   printf("Page frame: 0x%x\n", page_frame_number);

   // Find the difference from the buffer to the page boundary
   unsigned int distance_from_page_boundary = (unsigned long)buffer % getpagesize();

   // Determine how far to seek into memory to find the buffer
   uint64_t offset = (page_frame_number << PAGE_SHIFT) + distance_from_page_boundary;

	 unsigned long physcial_addr = (page_frame_number << PAGE_SHIFT) + distance_from_page_boundary;
 	printf("Virtual Addr %p Physical Addr %p\n",&buffer,physcial_addr);
}



int main()
{

  void* var=malloc(sizeof(int));
 // Lock the page in memory
	//The kernel does not gaurentee that an address in the virtual address space actually maps to a physical address in memory.
	// It may be stored in the swap space, in a cache somewhere, or somewhere else entirely. 
	//To get around this possibility, we can use mlock() to force a page to be kept in the physcial memory of the system.
   // Do this before writing data to the buffer so that any copy-on-write
   // mechanisms will give us our own page locked in memory
   if(mlock(var, sizeof(int)) == -1)
    {
      fprintf(stderr, "Failed to lock page in memory: %s\n", strerror(errno));
      exit(1);
   }

   // Add some data to the memory
   //strncpy(var, 5,sizeof(int));
      var=5;
  VtoP(var);

	return 0;

}