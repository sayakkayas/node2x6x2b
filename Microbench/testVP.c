#include <stdio.h>
#include <stdlib.h>
#include <asm-i386/page.h>
#define virt_to_page(kaddr)     pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)
int main()
{

  unsigned long res;
  int count=0;

  res=virt_to_page(&count);

	return 0;
}