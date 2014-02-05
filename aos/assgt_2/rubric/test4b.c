#include<stdio.h>
#include<gtthread.h>

gtthread_t* thread1;


void thread2fn(gtthread_t* thread)
{
  
  if(gtthread_equal(*thread1, *thread) != 0)
  {
  	fprintf(stderr, "\nSame");
  }
  else
  {
 	fprintf(stderr, "\nDiff");
  }
}


void thread1fn(gtthread_t* thread)
{
  
  	fprintf(stderr, "\nDummy");
}

int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t* thread2; 
 
  gtthread_init(50000);
  
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &thread1fn, thread1);

  thread2 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread2, &thread2fn, thread2);

  while(1)
  {;
  }	

  return 0;
}

