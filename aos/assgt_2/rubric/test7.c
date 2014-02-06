#include<stdio.h>
#include<gtthread.h>

gtthread_mutex_t* mutex;
int counter = 0;

void thread1fn()
{
	
	if(counter > 1)
	{
		fprintf(stderr, "\Im in Counter: %d", counter);
		exit(1);
		
	}
	counter++;
	gtthread_t* thread2, *self;
	
	thread2 = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread2, &thread1fn, counter);
	self = gtthread_self();

	int  i = 0;

	while(i< 10)
	{
		fprintf(stderr, "\nLOG: Thread %d %d", self->threadID , i);
		i++;
	}

	while(i < 99999)
		i++;	
}


int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t* thread1;
 
  gtthread_init(50);
	 
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &thread1fn, NULL);

  fprintf(stderr, "im here");
  
  while(1)
  {;}
  return 0;
}

