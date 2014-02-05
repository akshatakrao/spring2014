#include<stdio.h>
#include<gtthread.h>

gtthread_mutex_t* mutex;

void thread1fn()
{

	gtthread_yield();
	fprintf(stderr, "Thread 1 resume after yield");

}

void thread2fn()
{
	gtthread_yield();
	fprintf(stderr, "Thread 2 resume after yield");

}


int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t* thread1, *thread2;
 
  gtthread_init(50000);
	 
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &thread1fn, NULL);

  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread2, &thread2fn, NULL);

  fprintf(stderr, "Thread 1 state %d", thread1->state);
  gtthread_join(*thread1, &returnVal);
  gtthread_join(*thread2, &returnVal);
	
  fprintf(stderr, "im here");
  
  return 0;
}

