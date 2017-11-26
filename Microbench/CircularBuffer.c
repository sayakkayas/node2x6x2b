/*Created by Sayak Chakraborti for testing a circular bounded FIFO buffer*/

#include <stdio.h>
#include <stdlib.h>

#define MAX 10

struct buffer_struct
{
	int buffer[MAX];
	int rear;
	int front;

};


int main()
{

 struct buffer_struct s; 
 initialize_buffer(&s);

 int count =0;
 while(count<100)
 { 
 	int r1=rand()%2;

 	if(r1==0)
 	{
 		int e=rand()%100;
 		printf("Insert %d into buffer\n",e);
 		int r=insert_into_buffer(&s,e);

 	}
    else
     { printf("Attempting to delete value from buffer\n");
     	int val=delete_from_buffer(&s);
         printf("Value deleted %d\n",val);
      }	

     printf("Front %d Rear %d\n",s.front,s.rear);
     printf("\n");
    //scanf()
 	count++;
 }


return 0;
}
void initialize_buffer(struct buffer_struct* buf)
{
	buf->rear=-1;
	buf->front=-1;

}

int insert_into_buffer(struct buffer_struct * buf,int val)
{
	if(buf->rear==MAX-1 && buf->front==0)
	{
		printf("Buffer full\n");
		return 0;
	}
	else
	{
		if((buf->rear + 1)== (buf->front) )
		{

           printf("Buffer full\n");
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

   buf->buffer[buf->rear]=val;		

 return 1;

}

int delete_from_buffer(struct buffer_struct * buf )
{

   if(buf->front==-1 && buf->rear==-1)
   {
   	  printf("Buffer is empty\n");
   	   return -1;
   }
   else
   {
       if(buf->front==buf->rear)
       { int e=buf->buffer[buf->front];
         buf->front=-1;
         buf->rear=-1;
         return e; 
       }
       else
       {
   	   		if(buf->front==MAX-1 && buf->rear!=MAX-1)
   	   		{
   	   			int e=buf->buffer[buf->front];
   	   	 		buf->front=0;
         		return e;
   	   		}
   	   		else
   	   		{	int e=buf->buffer[buf->front];
   	   			buf->front=buf->front+1;
        		return e;
        	}

      
       }

   }

}   