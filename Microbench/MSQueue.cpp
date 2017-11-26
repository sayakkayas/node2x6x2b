#include <thread>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <atomic>
#include <mutex>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <utility>
using namespace std;


struct pointer_t
{

 struct node_t* ptr;
 unsigned int count;

};


struct node_t
{
 int data;
std::atomic<struct pointer_t*> next;

};

struct queue_t
{

struct pointer_t  Head;
struct pointer_t Tail;

};



void initialize(struct queue_t* Q)
{
 struct node_t* node;
 node=(struct node_t*)malloc(sizeof(struct node_t*));
 struct pointer_t temp;
 temp.ptr=NULL;
 temp.count=0;
 node->next.store(temp);
 Q->Head.ptr=node;
 Q->Tail.ptr=node;


}


void enqueue(struct queue_t* Q,int val)
{
  struct node_t* node;
   node=(struct node_t*)malloc(sizeof(struct node_t*));
   struct pointer_t temp;
   temp.ptr=NULL;
   node->next.store(temp);
   node->data=val;

  while(1)
  {
     struct pointer_t tail,next;
     tail=Q->Tail;
     next=tail.ptr->next;

     if(tail.ptr==(Q->Tail).ptr && tail.count==(Q->Tail).count )
     {
        if(next.ptr==NULL)
         {
           //use
           struct pointer_t change;
          // change=(struct pointer_t*)malloc(sizeof(struct pointer_t));
           change.ptr=node;
           change.count=next.count+1;

             if ( tail.ptr->next.compare_exchange_strong(&next,change,memory_order_release,memory_order_acquire) )  //# Try to link node at the end of the linked list
              break;


         }


     }


  }


}

int main()
{

return 0;
}
