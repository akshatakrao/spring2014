#include<gtthread_test.h>
#include<stdio.h>
#include<gtthread_queue.h>
#include<gtthread_list.h>
#include<ucontext.h>
#include<stdlib.h>
//#include<gtthread_scheduler.h>
#include<string.h>

int trial()
{
	fprintf(stderr,"\nLOG: Whats up MAD BOY");
}

void test_init()
{
    gtthread_t *thread;
    int a = 3;
    void* arg  = &a;
    gtthread_init(100000);
    fprintf(stddebug,"\nLOG: Whatsup?");

    thread = (gtthread_t*)malloc(sizeof(gtthread_t));
    gtthread_create(thread, &trial, arg);
}

void createDebugFile()
{
  stddebug = stderr;//fopen("debug.log", "w");

}

void closeDebugFile()
{
    fclose(stddebug);
}
void test_create()
{
/*	gtthread_t *thread, *thread1;
	int a = 3;
	void *arg;

	arg = &a;
  gtthread_init(500000);

	thread = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread, &trial, arg);
	
	displayList(listOfThreads);
	displayQueue(readyQueue);	
	
	thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread1, &trial, arg);

	displayList(listOfThreads);
	displayQueue(readyQueue);
*/
//	setcontext(&(thread->context));
}


