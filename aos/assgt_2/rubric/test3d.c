#include<stdio.h>
#include<gtthread.h>

gtthread_t* thread1;


void thread1fn()
{
   int i = 0;
   while(i < 9999999)
  {	
  	i++;
  }
 
  fprintf(stderr, "\nThread 1 exiting");
}

void thread2fn()
{
   void* retVal;
  
   gtthread_join(*thread1, retVal);
   
   fprintf(stderr, "\nThread 2 exiting");
}

int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t *thread2;
  
  gtthread_init(50000);
  
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &thread1fn, 32);

  thread2 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread2, &thread2fn, 64);
	  
  while(1)
  {;
  }	

  return 0;
}

