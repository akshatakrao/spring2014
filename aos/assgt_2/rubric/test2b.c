#include<stdio.h>
#include<gtthread.h>

char* printHello(int a)
{

  fprintf(stderr, "\nThread 1 exiting %d ", a);

  gtthread_exit("im awesome");		
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

  fprintf(stderr,"\nReturn value: %s", (char*)returnVal);

  while(1)
  {
  ;
  }

  return 0;
}

