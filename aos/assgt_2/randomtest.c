#include<stdio.h>
#include<gtthread.h>

gtthread_mutex_t* mutex;

void thread1fn()
{
	int i = 0;
	gtthread_mutex_lock(mutex);	
	gtthread_mutex_unlock(mutex);

	gtthread_mutex_lock(mutex);	
	fprintf(stderr, "Im in thread1");	
	gtthread_mutex_unlock(mutex);

}

void thread2fn()
{
	int i = 0;
	gtthread_mutex_lock(mutex);	
	gtthread_mutex_unlock(mutex);

	while(i< 9999)
	{
		i++;
	}

	gtthread_mutex_lock(mutex);	
	fprintf(stderr, "Im in thread2");	
	gtthread_mutex_unlock(mutex);

}


int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t* thread1, *thread2;
 
  gtthread_init(50000);
	 
  mutex = (gtthread_mutex_t*)malloc(sizeof(gtthread_mutex_t));	
  gtthread_mutex_init(mutex);	  

  gtthread_mutex_lock(mutex);	

  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &thread1fn, NULL);

  thread2 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread2, &thread2fn, NULL);

  fprintf(stderr, "Im in Main");	
  gtthread_mutex_unlock(mutex);

  	
  gtthread_join(*thread1, &returnVal);
  gtthread_join(*thread2, &returnVal);
	
  fprintf(stderr, "im here");
  
  return 0;
}

