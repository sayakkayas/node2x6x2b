/*Producer consumer with multiple threads-Created by Sayak Chakraborti as a benchmark for projects
Try both-Shared  Memory and Message Passing

Shared Memory
 _    _    _
|A| /|B| /|C|
|B|/ |C|/ |D|

---------------------
---------------------

Message Passing
 _    _    _ 
|A| /|C| /|E|
|B|/ |D|/ |F|

NOTE: IMP if iterations > size of last consumer then deadlock
*/
//Compile-gcc ProducerConsumerBenchMark.c -lpthread -o PCB -w
/*Note-act of inserting/taking out from producer consumer buffers should be mutually exclusive to prevent races */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#define MAX 100

int start=0; //wait variable for threads
pthread_t* thread;


struct buffer_struct
{
   char buffer[MAX];
   pthread_mutex_t lock;
   int rear;
   int front;

};

struct buffer_struct* PC;

struct arg_struct
{
   pthread_t tid;
   struct buffer_struct* Producer;
   struct buffer_struct* Consumer;

};

struct arg_struct * args;


int main(int argc,char* argv[])
{

   int n;

    if(argc < 2)
    {
      printf("Program parameters invalid,Try './PCB 4'\n");
    }
    else
    {

      n=atoi(argv[1]);
    }

	
   int r=create_threads(n);
    
    if(r==0)
    {
      printf("Problem creating threads,existing program\n");
      exit(0);
    }
  
	return 0;
}



void printPC(struct buffer_struct* Producer,struct buffer_struct* Consumer,pid_t tid )
{
  printf("Thread %d\n",tid);


   if(pthread_self()!=thread[0])
    { printf("Producer\n");
       display_buffer(Producer);
     }
  printf("Consumer\n");
  display_buffer(Consumer);  

}

void display_buffer(struct buffer_struct* buf)
{
  int i;
  for(i=buf->front;i<= (buf->rear);i++)
    printf("%d\n",buf->buffer[i]); 
}
//void * ProducerConsumer(int* Producer,int* Consumer)
void * ProducerConsumer(void* arguments)
{

	while(start!=1); //spin till all threads have been initialized
 
  struct arg_struct* arg=arguments;

  arg->tid=pthread_self(); //get own's thread ID
  
  //printPC(arg->Producer,arg->Consumer,arg->tid);
  int count=100;

   while(count>0)
   {
     count--;
      if(arg->tid == thread[0])
      {
          //No dependent producer,but the starter of the chain

         int produced_value=rand()%100;

          //keep trying to insert into consumer buffer
         int ret=1;
         do
         {
            ret=insert_into_buffer(arg->Consumer,produced_value);
 
          }
          while(ret==0);
       
           //Thread 0's consumer is thread 1's producer
     
        }
        else
        {

              //delete from producer

      
             int ret_val;         //Keep trying till we get a value (value -1 means buffer was empty)
            do
            {
                ret_val=delete_from_buffer(arg->Producer);
 
            }
              while(ret_val==-1);
       
          //insert into consumer

            int ret=1;
            do                                //keep trying till you can enter your value into the buffer
            {
                ret=insert_into_buffer(arg->Consumer,ret_val);
 
            }
            while(ret==0);
       

        }

   }//while close   
}


void initialize_buffer(struct buffer_struct* buf)
{
  buf->rear=-1;
  buf->front=-1;

}



int delete_from_buffer(struct buffer_struct * buf )
{

  pthread_mutex_lock(&(buf->lock));
  
   if(buf->front==-1 && buf->rear==-1)
   {
      //printf("Buffer is empty\n");

      pthread_mutex_unlock(&(buf->lock));
       return -1;
   }
   else
   {
       if(buf->front==buf->rear)
       { int e=buf->buffer[buf->front];
         buf->front=-1;
         buf->rear=-1;
         pthread_mutex_unlock(&(buf->lock));
         return e; 
       }
       else
       {
          if(buf->front==MAX-1 && buf->rear!=MAX-1)
          {
            int e=buf->buffer[buf->front];
            buf->front=0;
            pthread_mutex_unlock(&(buf->lock));
            return e;
          }
          else
          { int e=buf->buffer[buf->front];
            buf->front=buf->front+1;
            pthread_mutex_unlock(&(buf->lock));
            return e;
          }

      
       }

   }

}   

int insert_into_buffer(struct buffer_struct* buf,int value)
{

  pthread_mutex_lock(&(buf->lock));
   
   if(buf->rear==MAX-1 && buf->front==0)
  {
    //printf("Buffer is full\n");
    pthread_mutex_unlock(&(buf->lock));
    return 0;
  }
  else
  {
        if((buf->rear + 1)== (buf->front) )
        {

           //printf("Buffer  is full\n");

          pthread_mutex_unlock(&(buf->lock));
          return 0;

        }
        else
        { 
            if(buf->rear==-1 && buf->front==-1)
            {  
               buf->rear=0;
               buf->front=0;

            }
            else
            { 

                 if(buf->rear==MAX-1 && buf->front!=0 )
                  {
                    buf->rear=0;
 
                  }
                  else
                    buf->rear=buf->rear+1;
             }
        } 

  }

   buf->buffer[buf->rear]=value;    

 pthread_mutex_unlock(&(buf->lock));
 return 1;



  
}

int create_threads(int n)
{
   int i;
   thread=(pthread_t*)malloc(n*sizeof(pthread_t));
   args=(struct arg_struct*)malloc(n*sizeof(struct arg_struct));
   PC=(struct buffer_struct*)malloc(n*sizeof(struct buffer_struct));
  
   for(i=0;i<n;i++)
    initialize_buffer(&PC[i]);


   start=0;
   for(i=0;i<n;i++)
    {
         if(i!=0)
         { args[i].Producer=&PC[i-1];
           args[i].Consumer=&PC[i];

         }
         else
          args[i].Consumer=&PC[i];


        if(pthread_create(&thread[i],NULL,ProducerConsumer,(void*)&args[i]))
        	 {
        	 	 return 0;
        	 }
           else
            printf("Thread %d created with thread ID %d \n",i,thread[i]);

    }

   start=1;

   for(i=0;i<n;i++)
    pthread_join(thread[i],NULL);

   // printf("Final Consumer\n");
  //  display_buffer(&PC[n-1]);

    return 1;

}
