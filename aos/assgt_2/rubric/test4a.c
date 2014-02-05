#include<stdio.h>
#include<gtthread.h>

gtthread_t* thread1;


void thread1fn(gtthread_t* thread)
{
  
  if(gtthread_equal(*thread1, *thread) != 0)
  {
  	fprintf(stderr, "\nSame");
  }
}

int main()
{

  int a = 5;	
  void* returnVal;	  
  
  gtthread_init(50000);
  
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &thread1fn, thread1);

  while(1)
  {;
  }	

  return 0;
}

