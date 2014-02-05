#include<stdio.h>
#include<gtthread.h>

gtthread_t* thread1;


void thread2fn()
{
  
  if(gtthread_equal(*thread1, gtthread_self()) != 0)
  {
  	fprintf(stderr, "\nSame");
  }
  else
  {
 	fprintf(stderr, "\nDiff");
  }
}


int main()
{

  int a = 5;	
  void* returnVal;	  
 
  gtthread_init(50000);
  
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &thread2fn, NULL);

  while(1)
  {;
  }	

  return 0;
}

