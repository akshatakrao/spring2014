#include<stdio.h>
#include<gtthread.h>

void printHello(int a)
{
   while(1)
  {	
  	fprintf(stderr, "\nThread 1 %d ", a);
  }
 
}

int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t *thread;
  
  gtthread_init(50000);
  
  thread = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread, &printHello, 32);

  gtthread_join(*thread, &returnVal);

  
  fprintf(stderr, "\nLOG:Main exiting");
  

  gtthread_exit(NULL);

  return 0;
}

