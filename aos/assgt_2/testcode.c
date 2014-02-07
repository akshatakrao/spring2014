// Test2
// Join multiple threads, and check values returned by gtthread_exit.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gtthread.h>

void* worker1(void* arg)
{
	long count = (long)arg;
	if((count - 1) != 0)
	{
		gtthread_t thr;
		gtthread_create(&thr, worker1, (void*)(count-1));
	}
	int i;
	for(i=0; i<10; i++)
	{
		int j = 0;
		while(j < 666666)
		{
			j++;
		}
		printf("Hi: %lu\t", count);
	}
	return (void*)0;
}

int main()
{
	gtthread_t thread1;
	gtthread_init(1000);
	gtthread_create(&thread1, worker1, (void*)4);
	gtthread_exit(0);
	return 0;
}

