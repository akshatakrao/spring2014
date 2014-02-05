#include<stdio.h>
#include<gtthread.h>

void printHello(int a)
{
  fprintf(stderr, "\nHello WOrld %d ", a);
//  exit(1);
}

int main()
{

  int a = 5;	
  gtthread_t *thread;
  
  gtthread_init(50000);
  
  thread = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread, &printHello, 32);


  while(1)
  {
  ;
  }

  return 0;
}

