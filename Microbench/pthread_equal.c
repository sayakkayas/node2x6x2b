#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdatomic.h>

pthread_t threads[2];
int start=0;

 struct arg_struct
{
   pthread_t tid;
  atomic_int* counter_ptr;

};

struct arg_struct  args[2];



void* doSomeThing(void *arguments)
{

    while(start==0);
    struct arg_struct* arg=arguments;
    //unsigned long i = 0;
    pthread_t id = pthread_self();

    if(pthread_equal(id,threads[0]))
    { 
        while(*(args->counter_ptr)<100)
         {   
            printf("\n First thread processing...\n");
       
            if(*(args->counter_ptr)%2==0)
            atomic_fetch_add((args->counter_ptr),1);
            printf("Counter %d\n",*(args->counter_ptr));

         }
    }
    else
       { 


         while(*(args->counter_ptr)<100)
         {
           printf("\n Second thread processing...\n");
           if(*(args->counter_ptr)%2==1)
           atomic_fetch_add((args->counter_ptr),1);
           printf("Counter %d\n",*(args->counter_ptr));

          }

        }
	//usleep(5);
    return NULL;
}

int main(void)
{
    int i = 0;
    int err;


    atomic_int counter=0;
  

   args[0].counter_ptr=&counter;
   args[1].counter_ptr=&counter;
  
  
    for (i = 0; i < 2; i++)
     {
        err = pthread_create(&(threads[i]), NULL, &doSomeThing, (void*)&args[i]);
        if (err != 0)
            printf("\n Can't create thread :[%s]\n", strerror(err));
        else
            printf("\n Thread %d created successfully with thread ID %d.\n", i,threads[i]);
    }

     start=1;

    for (i = 0; i < 2; i++)
        pthread_join(threads[i], NULL);

    return 0;
}
