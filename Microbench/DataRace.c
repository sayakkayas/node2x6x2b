#include<stdio.h>
#include<string.h>
#include<pthread.h>

static int count;


void *threadFunc1()
{
 int x=count;
 count=x+1;

}

void *threadFunc2()
{
 int x=count;
 count=x+1;

}

int main()
{
    pthread_t thr1;
    pthread_t thr2;

     count =0;

      //creating two threads

      if(pthread_create(&thr1,NULL,threadFunc1,NULL))
      	{ printf("Error creating thread 1");
          exit(0);
        }

      if(pthread_create(&thr2,NULL,threadFunc2,NULL))
      	{ printf("Error creating thread 1");
          exit(0);
        }

   //waiting for the threads to terminate
    pthread_join(thr1,NULL);
    pthread_join(thr2,NULL);

    printf("Count %d\n",count);

	return 0;
}