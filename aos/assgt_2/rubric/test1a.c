#include<stdio.h>
#include<gtthread.h>

void printHello()
{
  fprintf(stderr, "\nHello WOrld");
//  exit(1);
}

int main()
{

  gtthread_t *thread;
  
  gtthread_init(50000);
  
  thread = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread, &printHello, NULL);


  while(1)
  {
  ;
  }

  return 0;
}

