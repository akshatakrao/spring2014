#include<gtthread_test.h>
#include<stdio.h>
#include<gtthread_queue.h>
#include<gtthread_list.h>
#include<ucontext.h>
#include<stdlib.h>
//#include<gtthread_scheduler.h>
#include<string.h>

int trial1()
{
	fprintf(stderr,"\nLOG: In trial thread 1");
}

int trial2()
{
    fprintf(stderr, "\nLOG: In trial thread 2");
}

int trial3()
{
    fprintf(stderr, "\nLOG: In trial thread 3");
}

void test_init()
{
    gtthread_t *thread3, *thread1, *thread2;
    int a = 3;
    void* arg  = &a;
    gtthread_init(50000);

    thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
    fprintf(stddebug, "\nLOG: Adding thread 1");
    
    gtthread_create(thread1, &trial1, arg);
    fprintf(stddebug, "\nLOG:Added Thread 1. Adding thread 2"); 
  
    fprintf(stddebug, "\nLOG: Threads are equal? %d", gtthread_equal(*thread1, *thread1)); 
    thread2 = (gtthread_t*)malloc(sizeof(gtthread_t)); 
    gtthread_create(thread2, &trial2, arg); 
    fprintf(stddebug, "\nLOG: Added Thread 2. Adding thread 3");
 
    fprintf(stddebug, "\nLOG: Diff Threads are equal? %d", gtthread_equal(*thread1, *thread2));   
    thread3 = (gtthread_t*)malloc(sizeof(gtthread_t));
    gtthread_create(thread3, &trial3, arg);
    //fprintf
    //
    fprintf(stddebug, "\nHERE!");
    /*while(1)
    {
        fprintf(stddebug, "Whats up");
    }*/
    fprintf(stddebug, "\nHERE @");   
    
    sleep(10); 
  /*   while(1)
    {
       // printf("hi\t"); 
    }*/
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

void test_exit()
{
    int retVal = 1, a = 3;

    gtthread_init(5000000);
    gtthread_exit(&retVal);
}

gtthread_t *joinee;

void joinFunction()
{
    fprintf(stddebug, "\nLOG: Im in JOIN");
    void **retVal;
  

    gtthread_join(*joinee, retVal);
    fprintf(stddebug, "\nAfter Join Returns");
}
void test_join()
{
  gtthread_t *thread1, *thread2;
	int a = 3;
	void *arg;

	arg = &a;
  gtthread_init(50000);

	thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(thread1, &joinFunction, arg);

  joinee = (gtthread_t*)malloc(sizeof(gtthread_t));
	gtthread_create(joinee, &trial1, arg);
   
  fprintf(stderr, "\nWhats up?");
  while(1)
  {
  } 

}
