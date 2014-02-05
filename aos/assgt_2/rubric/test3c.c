#include<stdio.h>
#include<gtthread.h>

void printHello(int a)
{
   int i = 0;

    while(i < 5000)
   {
	fprintf(stderr,"\nLOG: Thread %d hello", a);
	i++;	
   }
 
}

int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t *thread1, *thread2;
  
  gtthread_init(10000);
  
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread1, &printHello, 1);

  thread2 = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread2, &printHello, 2);
  //gtthread_join(*thread, &returnVal);


  while(1)
  {
   ;
  } 	
	
  return 0;
}

