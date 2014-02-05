#include<stdio.h>
#include<gtthread.h>

char* printHello(int a)
{

  fprintf(stderr, "\nHello WOrld %d ", a);

  return "akshata";		
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
  	
  fprintf(stderr, "\nMy Ret Value: %s", (char*)returnVal);	
  while(1)
  {
  ;
  }

  return 0;
}

