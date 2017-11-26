#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#define BUFFER_MAX 102400

 int buffer[BUFFER_MAX];
 int in=0;
 int out=0;


pthread_mutex_t lock;




void Producer()
{
 
	while(1)
 	{
            //produce an item
             int item=rand()%100;
          
             if(in==BUFFER_MAX-1)
              break;
             else
               {
                 pthread_mutex_lock(&lock);
                  buffer[in++]=item; 
                 
                pthread_mutex_unlock(&lock);
                      

               }
           }

}


void Consumer()
{
 
	while(1)
 	{
            //produce an item
             int item;
          
             if(out==BUFFER_MAX-1)
              break;
             else
               {
                 pthread_mutex_lock(&lock);
                  item=buffer[out++]; 
                 
                pthread_mutex_unlock(&lock);
                      

               }
           }

}


int main()
{

int i,err;
pthread_t threads[2];

 for(i=0;i<2;i++)
 {

  if(i==0)
  err=pthread_create(&threads[i],NULL,Producer,NULL);
  else
  err=pthread_create(&threads[i],NULL,Consumer,NULL);
 
  sleep(2);
 }

 for(i=0;i<2;i++)
 {
   pthread_join(threads[i],NULL);
 }
return 0;
}
