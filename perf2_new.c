/*Compile-gcc perf2.c -o perf_EXC -w
 * Run-sudo ./perf_EXC //sudo privilege for sysmtem wide sampling
 */

/*Events on Intel XEon E5-2660
 * r10d3-Retired load uops whose data sources were remote HITM  //helps to understand cross sockect traffic as to cache line being constantly transfferd in M state
 * r20d3-Retired load uops who data sources where forwards from remote cache //??
 * r0Cd3-Retired load uops whose data source was a remote DRAM(snoop not needed,Snoop Miss or Snoop hit data not forwarded)//may help us understand single thread memory affinity and do memory migration and address trace using PEBS will help us locate which DRAM node we need to look into*/
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#define rmb() asm volatile("lfence":::"memory");

//int perf_event_open(struct perf_event_attr *attr,pid_t pid, int cpu, int group_fd,unsigned long flags);
 
#define NUM_EVENTS 3  //maximum can be 10 as per the program design
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

struct thread_info
{
 char processID[20];
 char threadID[10];
 char mem_addr[UNIQUE][20];
 int freq[UNIQUE];
 int mem_addr_index; 
};

struct thread_info threads[THREAD_MAX];
int thread_index=0;

//bloom filters
#define M 2000
#define K 1
#define S1 2000 
#define S2 2000
#define MAX_FILTERS 20

struct Bfilter
{
         bool taken;
        pid_t threadID;
    bool filter[M];
    int frequency[M];

};

struct Bfilter s[MAX_FILTERS];


bool resFilter[M];
//Pairs
struct Pair
{
int i;
int j;
};

struct Pair P[100];
int pair_index=0;



static long  perf_event_open(struct perf_event_attr *hw_event, pid_t pid,int cpu, int group_fd, unsigned long flags)
       {
                  int ret;
                  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,group_fd, flags);
                 return ret;
        }

void init_resFilter()
{

  int i;
  for(i=0;i<M;i++)
  	resFilter[i]=false;

}

int main(int argc, char **argv)
     {
       // initialize_filters(s);

       while(1)
          {         
             //collect sample every 1000 occurence of the event
             system("perf stat  -o output.txt -a -e r10d3,r20d3,r0cd3  sleep 5   > output.txt");

            long  event_cnt[10];

             readOutputFile(&event_cnt); //function reads output file and returns event values

              //printf("%ld %ld\n",event_cnt[0],event_cnt[0]-100000);
              // print_evnt_cnts(event_cnt);
                 //int i;
                 //for(i=0;i<NUM_EVENTS;i++)
                 //printf("%ld\n",event_cnt[i]);
 

                 printf("Sampling period 5 seconds\n");
                 printf("r10d3-Retired load uops whose data sources were remote HITM Count=%ld\n",event_cnt[0]);  //based on this we are deciding on the cross socket traffic
                 printf("r20d3-Retired load uops who data sources where forwards from remote cache Count=%ld\n",event_cnt[1]); //??
                 printf("r0Cd3-Retired load uops whose data source was a remote DRAM(snoop not needed,Snoop Miss or Snoop hit data not forwarded) Count=%ld\n",event_cnt[2]);

              //printf("%ld %ld\n",event_cnt[0],event_cnt[0]-100000);

             if(event_cnt[0] > THRESHOLD_VAL)
              {
                  initialize_filters();
                     pair_index=0;                  

               printf("SOMETHING NEEDS TO BE DONE\n");
               printf("Sample for addresses using PEBS\n");
               sample_addresses();
                printf("Populate thread specific partitioned bloom filters done\n");
               printf("Display Thread INFO\n\n");
               display_threadInfo();

               printf("\n");
              printf("Display Filter INFO\n");
                display_filterInfo();
               printf("Filter intersection\n");
               check_intersection_filter();       
               
               printf("Intersect the bloom filters to find sharing ratio(defined properly)\n");
               printf("Schedule accordingly so that cross socket traffic is reduced\n");
                


              }
              else
              {

                  printf("NOTHING TO BE DONE.\n");
              }
 
              printf("\n\n");
         }          

       printf("EXIT.\n");

       return 0;
      }

/*
struct Pair
{
int i;
int j;
};

struct Pair P[100]
int pair_index=0;
*/
void insertPair(int i,int j)
{

  P[pair_index].i=i;
 P[pair_index].j=j;
 
 pair_index++;

}

bool checkPair(int i,int j)
{

 int k;
   for(k=0;k<pair_index;k++)
   {
      if( (P[k].i==i && P[k].j==j ) || (P[k].j==i && P[k].i==j))
      return false;

   }

 return true;
}


bool checkInRange(int start,int end)
{
 int i;
  for(i=start;i<end;i++)
  	 {
  	 	if(resFilter[i]==true)
  	 		return true;
  	 }
return false;
}

bool checkRESFILETER()
{
   int i;
   bool res=true;
  for(i=0;i<K;i++)
    { int range=i*(M/K);

      res=res&checkInRange(range,range+(M/K));
    }   
    	
return res;

}


void check_intersection_filter()
{

  int i,j;
  
  int intersection_bit_count=0;
  int set_in_filter1=0;
 int set_in_filter2=0;
 long int intersection_frequency_count=0;
 long int fr_in_filter1=0;
 long int fr_in_filter2=0;
 long int intersect_fr_in_filter1=0;
 long int intersect_fr_in_filter2=0;

   for(i=0;i<MAX_FILTERS;i++)
   {
         if(s[i].taken==false)
            break;

      for(j=0;j<MAX_FILTERS;j++)
      {
           if(s[j].taken==false)
             break;

           if(i!=j && checkPair(i,j) )
           {
                    insertPair(i,j);

                int k;
                   for(k=0;k<M;k++)
                    {
                                   if(s[i].filter[k])
                                    { set_in_filter1++;
                                       fr_in_filter1=fr_in_filter1+s[i].frequency[k];
                                    }
 
                                      if(s[j].filter[k])
                                       { set_in_filter2++;
                                         fr_in_filter2=fr_in_filter2+s[j].frequency[k];

                                       }

                                 if(s[i].filter[k] & s[j].filter[k])
                                    {  resFilter[k]=true;
                                                  intersection_bit_count++;
                                      intersect_fr_in_filter1=intersect_fr_in_filter1+s[i].frequency[k];
                                      intersect_fr_in_filter2=intersect_fr_in_filter2+s[j].frequency[k];
                                     }
                    }
                            
      

      if(checkRESFILETER())
       {	
        printf("Filter intersection  between TID %d and TID %d : %d     Percentage of %d in intersection:%.2lf%  Percentage of %d in intersection:%.2lf%   Percentage as whole:%.2lf%\n", s[i].threadID,s[j].threadID,intersection_bit_count,s[i].threadID,(double)intersection_bit_count/(double)set_in_filter1*100.00,s[j].threadID,(double)intersection_bit_count/(double)set_in_filter2*100.00,(double)intersection_bit_count/(double)M*100.00);
        printf(" Percentage frequency in intersection: Thread %d: %.2lf%  Thread %d:   %.2lf%\n",s[i].threadID,(double)intersect_fr_in_filter1/(double)fr_in_filter1*100.00,s[j].threadID,(double)intersect_fr_in_filter2/(double)fr_in_filter2*100.00);
        printf("Total frequency: Frequency in inter || Thread %d: %ld : %ld     Thread %d:%ld :%ld\n\n",s[i].threadID,fr_in_filter1,intersect_fr_in_filter1,s[j].threadID,fr_in_filter2,intersect_fr_in_filter2);        
       }
       else
       	printf("Filter intersection  between TID %d and TID %d :NO INTERSECTION\n",s[i].threadID,s[j].threadID);

       intersection_bit_count=0;
         intersection_frequency_count=0;
         fr_in_filter1=0;
         fr_in_filter2=0;
        intersect_fr_in_filter1=0;
        intersect_fr_in_filter2=0;
          set_in_filter1=0;
          set_in_filter2=0;
          init_resFilter();
             }


      }

      
   } //for i close




}

void check_intersection()
{
  int i,j;
  

  int intersection_count=0;
 
  for(i=0;i<thread_index;i++)
  {
     for(j=0;j<thread_index;j++)
      {

           int k,l;

            if(i!=j)
             {
 
                    for(k=0;k<threads[i].mem_addr_index;k++)
                      {

                         for(l=0;l<threads[j].mem_addr_index;l++)
                            {
                                 if( strcmp(threads[i].mem_addr[k],threads[j].mem_addr[l])==0)
                                   intersection_count++;

 
                            }
                        

                      }             


            
    
          printf("Intersection between PID %s TID %s and PID %s TID %s : %d\n",threads[i].processID,threads[i].threadID,threads[j].processID,threads[j].threadID,intersection_count);
         intersection_count=0;
               }//if close

      } //j close
 
  } //i close


 
   
}
void sample_addresses()
{
  //mem- profile memory acceses
   system("perf mem -D record --per-thread -a -e r10d3 -c 10000  sleep 2");  //record memory addresses accessed sys
   printf("Memory acceses for event r10d3 profiled for 2 secs,sample every 1000 occurence of the event\n");
  system("perf script  > perf_script_out.txt");
 printf("Profile written into perf_script_out.txt\n"); 
 system("perf mem  report > mem_report.txt");
  printf("Profile written to mem_report.txt\n");
  system("perf mem -D report > mem_D_report.txt");
  printf("Profile written to mem_D_report.txt\n");
   // parse_perf_script();

 //  display_threadInfo();

read_mems();
//display_threadInfo();


}



void process_string(char* string,int line_no )
{
  
 char process_ID[10];
char thread_ID[10];
char mem_addr[30];

 char* pch;
 pch=strtok(string," ");
 int flag=0;
  char mem[20];
 char TID[10];

  while(pch!=NULL)
  {
     
      if(flag==0)
      { strcpy(process_ID,pch);
        
        flag=1;
          pch=strtok(NULL," ");

        continue;
      }
  
        if(flag==1)
        {
          strcpy(thread_ID,pch);
         strcpy(TID,pch);
             flag=2;
           pch=strtok(NULL," ");

        continue;
            
        }
     
      if(flag==2)
        {
            flag=3;
           pch=strtok(NULL," ");

        continue;
            
        }
     
       if(flag==3)
        {
          strcpy(mem_addr,pch);
          strcpy(mem,pch);
          flag=4;
           pch=strtok(NULL," ");

        break;
            
        }
     
    
  } 



if(flag==4)
 { 

  // printf("Process name: %s   ",process_ID);
  /*
  if(strcmp(thread_ID,"0")!=0 && (mem_addr[2]=='f' && mem_addr[3]=='f' && mem_addr[4]=='f' && mem_addr[5]=='f') )
   printf("Thread ID: %s  Memory Address :%s Line number:%d\n",thread_ID,mem_addr,line_no);
  */
 
   //printf("\n");
	//populate_filter(TID,mem);
      //if( !(mem_addr[2]=='f' && mem_addr[3]=='f' && mem_addr[4]=='f' && mem_addr[5]=='f')  )
      if(strlen(mem_addr)<=14 && ( ((int)mem_addr[2]-48) <=7) )
      {
 

        if(strcmp("0",process_ID)!=0 || strcmp("0",thread_ID)!=0)
         {
            if(!(mem_addr[0]=='0' && mem_addr[1]=='x' && mem_addr[2]=='0'))
            {
              add_threadInfo(process_ID,thread_ID,mem_addr);
              populate_filter(TID,mem);
            }

          }
     } 

 }

}




void add_threadInfo(char* process_ID,char* thread_ID,char* mem_addr)
{

  /*
 *
 * struct thread_info
 * {
 *  char processID[10];
 *   char threadID[10];
 *    char mem_addr[UNIQUE][20];
 *     int mem_addr_index=0;
 *     };
 *
 *     struct thread_info threads[THREADS_MAX];
 *     int thread_index=0;
 */
 int flag2=0;
  int flag=0;
  
  int i=0;int j=0;
   for(i=0;i<thread_index;i++)
    {
        if(strcmp(threads[i].threadID,thread_ID)==0)
         { flag2=1;
           flag=0;
            for(j=0;j<threads[i].mem_addr_index;j++)
             {

                 if(strcmp(threads[i].mem_addr[j],mem_addr)==0)
                   {
                          threads[i].freq[j]+=1;
                       flag=1;
                     break;
                   }
  
             }

             
              if(flag==0)
             {
                 strcpy(threads[i].mem_addr[threads[i].mem_addr_index],mem_addr);
                 threads[i].freq[threads[i].mem_addr_index]=1;
                 threads[i].mem_addr_index++;
             }

          
         }


     } //for close


   if(flag2==0)
   {
         //add new thread_ID
       threads[thread_index].mem_addr_index=0;
     strcpy(threads[thread_index].processID,process_ID);
     strcpy(threads[thread_index].threadID,thread_ID);
     strcpy(threads[thread_index].mem_addr[0],mem_addr);
       threads[thread_index].freq[threads[thread_index].mem_addr_index]=1;
       threads[thread_index].mem_addr_index++;
    
     thread_index++;


   }

 }


void display_filterInfo()
{

printf("\n");

 int i;
for(i=0;i<MAX_FILTERS;i++)
 {
     if(s[i].taken==true)
     {
        printf("Filter index %d ThreadID %d\n",i,s[i].threadID);

     }
     else
      break;
 
 }

printf("\n");

}

void display_threadInfo()
{

/*struct thread_info
 * {
 *  char process_name[20];
 *   char threadID[10];
 *    char mem_addr[UNIQUE][20];
 *     int mem_addr_index=0;
 *     };
 *
 *     struct thread_info threads[THREADS_MAX];
 *     int thread_index=0;
 */

  int i;int j;
   for(i=0;i<thread_index;i++)
   {
    if(strcmp(threads[i].processID,"0")!=0)
     { printf("Process ID:%s ThreadID:%s\n",threads[i].processID,threads[i].threadID);
       for(j=0;j<threads[i].mem_addr_index;j++)
        printf("Mem addr:%s  Freq:%d\n",threads[i].mem_addr[j],threads[i].freq[j]);

        printf("Unique addresses count : %d\n",threads[i].mem_addr_index);
      printf("\n\n");
     }
   } 

}

void populate_filter(char threadID[10],char mem_addr[30])
{
        getfilters(threadID,mem_addr);   
 }


void read_mems()
{

   FILE* fp;
   fp=fopen("mem_D_report.txt","r");

   char g;
   char string[10000];
    int line_no=0;
    while((g=fgetc(fp))!=EOF)
    {
      fseek(fp,-1,SEEK_CUR);
    
        if( fgets(string,10000,fp)!=NULL)
           {
                line_no++;
                if(string[0]=='#')
                  continue;

                  //
                  //printf("String:\n%s\n",string);
                  //char process_ID[20];
                  //char thread_ID[10];
                 
                  //char mem_addr[20];
                 process_string(string,line_no);

            }



    }


    fclose(fp);




}

//BLOOM FILTERS


void initialize_filters()
{

  int i,j;
  for(i=0;i<MAX_FILTERS;i++)
  {
  	s[i].taken=false;
  	s[i].threadID=0;
        
       for(j=0;j<M;j++)
         { s[i].filter[j]=false;
           s[i].frequency[j]=0;
              
         }
  }
  

}

pid_t typeTID(char* cTID)
{
	int p=0;

	int i;
	for(i=0;i<strlen(cTID);i++)
		p=p*10+((int)cTID[i]-48);


return (pid_t)p;

}



long int typeAddr(char* cAddr)
{
	long int addr=0;

     int i;
	for(i=2;i<strlen(cAddr);i++)
		{
			if(cAddr[i] >='0' && cAddr[i] <='9' )
			{
			   addr=addr*10+((int)cAddr[i]-48);

            }
            else
            {
              if(cAddr[i] >='a' && cAddr[i] <='z' )
                {
                  addr=addr*10+((int)cAddr[i]-97+10);

                }
                else
                {
                  addr=addr*10+((int)cAddr[i]-65+10);                	
                }
            }

        }

	return addr;

}

int findIndex(pid_t tid)
{

int i;
   for(i=0;i<MAX_FILTERS;i++)
   {
       if(s[i].taken==false)
       	return i;
       else
       {
       	 if(s[i].threadID==tid)
       	 	return i;
       }

   }

   return -1;

}

int Hashk(long int addr,int num)
{

   num=(6*addr-5+(num*2+(num-1))+num%2)%M;

   return num;

}

bool check_filter(struct Bfilter s,long int addr,int part_width)
{


  int i;
  bool res=true;
  for(i=0;i<K;i++)
  {
  	   int range;
      		 range=i*part_width;
      		 //printf("Max range %d\n",range);
      		 int index_ret=Hashk(addr,i);
      		 index_ret=(index_ret)%part_width;
      		 index_ret=index_ret+range;
      		
            res=res & s.filter[index_ret];

  }
  /*
	if(s[index].filter[Hashk(addr)]==true)
	{

	}*/
  return res;
}


bool insert_into_filter(pid_t threadID,int index,long int addr,int part_width)
{
    /*
    if(check_filter(s[index],addr,part_width))
      {
             
	return true;
      } 
    else*/
    {
      int i;
      
     s[index].taken=true;
     s[index].threadID=threadID;
      for(i=0;i<K;i++)
      	{ 
      		 int range;
      		 range=i*part_width;
      		 //printf("Max range %d\n",range);
      		 int index_ret=Hashk(addr,i);
      		 index_ret=(index_ret)%part_width;
      		 index_ret=index_ret+range;
      		 //printf("Index in range %d\n",index_ret);
      		s[index].filter[index_ret]=true;
                s[index].frequency[index_ret]=s[index].frequency[index_ret]+1;

         }

    }
     


}



void getfilters(char* cTID,char* cAddr)
{
	pid_t tid=typeTID(cTID);
	long int addr=typeAddr(cAddr);


    int index=findIndex(tid);

    if(index!=-1)
    	{bool r=insert_into_filter(tid,index,addr,M/K);
    	}	

     

}


/*
void sample_addresses()
{


  struct perf_event_attr pe;
  int fd;

  memset(&pe, 0, sizeof(struct perf_event_attr));

  pe.type=PERF_TYPE_HW_CACHE;  //can be PERF_TYPE_HARDWARE or PERF_TYPE_HW_CACHE for my experiments
  pe.size=sizeof(struct perf_event_attr); //the size of perf_event_attr structure for forward/backward compatibility
  //config along with type specifies the type of event we want to sample
  pe.config=PERF_COUNT_HW_CACHE_L1D |( PERF_COUNT_HW_CACHE_OP_READ << 8 & PERF_COUNT_HW_CACHE_OP_WRITE << 8 ) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16) ;
 
  //record sample every 2000 memory loads(A sampling event generates an overflow notification every N events, N is defined by sample_period) 
  pe.sample_period=2000;
  //which information must be recorded in each sample
  //Various bits in this field specify which values to include in the sample.They will be stored in a ring-buffer which is available to the user space using mmap.
  //PERF_SAMPLE_IP-Records instruction pointer  PERF_SAMPLE_TID-Records the process and thread IDs 
  //PERF_SAMPLE_TIME-Records a timestamp     PERF_SAMPLE_ADDR-Records an address if applicable  PERF_SAMPLE_CPU-Records CPU number
  //PERF_SAMPLE_PERIOD-Records the current sampling period   PERF_SAMPLE_WEIGHT-Records a hardware provided weight which is analogous to the cost of the sample
  //PERF_SAMPLE_DATA_SRC-Records the data source :where in the memory hierarchy the data associated with the sampled instruction came from
  //
  pe.sample_type= PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME | PERF_SAMPLE_ADDR | PERF_SAMPLE_CPU | PERF_SAMPLE_PERIOD | PERF_SAMPLE_WEIGHT | PERF_SAMPLE_DATA_SRC;
  
  //PERF_FORMAT_READ-Allows all counter values to be read with one read
  //pe.read_format=PERF_FORMAT_GROUP;
  //disabled bit specifies whether the counter starts out disabled or not. disable=0 means it can start out immediately
  pe.disabled=1;
  //pe.inherit=1; 
  //the pinned bit specifies that the counter should always be on the CPU if at all possible.
   pe.pinned = 1;
  //The mmap bit enables generation of PERF_RECORD_MMAP samples every mmap call that has PROT_EXEC set
  pe.mmap = 1;
  //if this bit is set then the exit or fork information is included in the ring buffer
    pe.task = 1;
   pe.precise_ip=0; //SAMPLE_IP can have arbitary skid
 
  //if this bit is set then the count excludes the events that happened in kernel space
  pe.exclude_kernel = 1;
 //if this bit is set then the count excludes events that happened in the hypervisor
  pe.exclude_hv = 1;
 
  //pid_t TID=syscall(SYS_gettid);

   // pid == -1 and cpu >= 0
              This measures all processes/threads on the specified CPU.
              This requires CAP_SYS_ADMIN capability or a
              /proc/sys/kernel/perf_event_paranoid value of less than 1.
  

  //FLAGS- PERF_FLAG_FD_OUTPUT-This flag re-routes the output to the mmap buffer of the event specified  by group_fd

    // Make the call
     fd =perf_event_open(&pe,0,-1,-1,0);

  if (fd == -1) 
 {
   fprintf(stderr, "Error opening leader %llx\n", pe.config);
   exit(EXIT_FAILURE);
 }

 //Once the perf_event_open call has been issued, user code must map the memory containing the records in its address space. 
 //User code must also effectively start the sampling using another system call, namely ioctl
 // Map result, size should be 1+2^n pages
  size_t mmap_pages_count = 2048;
  size_t p_size = sysconf(_SC_PAGESIZE);
  size_t mmap_len = p_size *(1 + mmap_pages_count);
  struct perf_event_mmap_page* metadata = mmap(NULL, mmap_len, PROT_WRITE,MAP_SHARED, fd, 0);

  // Start sampling
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);

  ioctl(fd, PERF_EVENT_IOC_ENABLE,0);


  //samling code
  //int A=3;              //sample this basic instruction


  // Stop sampling
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

  // Analyze the samples
  uint64_t head_samp = metadata -> data_head;

  rmb();
  struct perf_event_header* header =metadata + p_size;
  uint64_t consumed = 0;

  while (consumed < head_samp)
  {
    if (header->type == PERF_RECORD_SAMPLE)
    {
        // Do something with the sample which 
        //like feed it to the partitioned bloom filters
        // fields start at address header + 8
     }
 
    consumed += header->size;
    printf("%s\n",consumed);
    header = (struct perf_event_header*)((char*)header + header -> size);
  }






}

*/



 void readOutputFile(long* event_cnt)
 { 

   FILE* fp;
   fp=fopen("output.txt","r");
   int evnt_cnt_index=0;
   char g;
   char string[10000];
   
    while((g=fgetc(fp))!=EOF)
    {
          fseek(fp,-1,SEEK_CUR);
         
        if( fgets(string,10000,fp)!=NULL)
          {
             int i;int flag=0;
             if(string[0]!='#')
             {

                   //check for numbers in the string
	             for(i=0;i<strlen(string);i++)
        	      {
                	  if(string[i] >= '0' && string[i] <='9')
                   	      {	 flag=1;
                     		 break;

                    	       }
                    	   else
                     	     {

                         	 if(string[i]!=' ')
                          	 {
                              		 flag=2;

	                           }


                  	    }

            	     }          
             

              if(flag==1)
               {
                  //printf("%s",string);
                  //remove the time string
                  //
                    int flag2=0;
                    long  num=0;
                    for(i=0;i<strlen(string);i++)
                     {

                            if(string[i]=='.')
                             {flag2=1;
                              break;
                             }
                             else
                             {
                              
                               if(string[i]=='r')
                                break;  

                               if(string[i]>='0' && string[i]<='9')
                                 num=num*10+(int)string[i]-48;

                             }

                     }

                     if(flag2==0)
                     { // printf("Num %d\n",num);
                       
                             if(evnt_cnt_index<NUM_EVENTS)
                             {
                                 event_cnt[evnt_cnt_index++]=num;

                              }
                  

                     }

               }
             
             }
        }
  
  }

  fclose(fp);



 }
                                               
