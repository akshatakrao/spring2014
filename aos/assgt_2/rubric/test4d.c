#include<stdio.h>
#include<gtthread.h>

gtthread_t* thread2;

void dummy()
{
fprintf(stderr, "\nWhatsup");
}
void thread1fn()
{
  
  if(gtthread_equal(*thread2, gtthread_self()) != 0)
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
  gtthread_t* thread1;
 
  gtthread_init(50000);
  
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &thread1fn, NULL);

  thread2 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread2, &dummy, NULL);

  while(1)
  {;
  }	

  return 0;
}

