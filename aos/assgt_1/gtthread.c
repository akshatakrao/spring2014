#include<stdio.h>
#include<gtthread.h>
#include<gtthread_queue.h>
#include<gtthread_list.h>
#include<ucontext.h>
#include<stdlib.h>

/*
 *
 *Init GTThreads
 */
void gtthread_init(long period)
{
    quantum_size = period;
    initQueue(&readyQueue);   //Initialize Ready queue 
    //TODO: Begin the Scheduler Process
    //
}


/**
 * Create a thread
 */
int  gtthread_create(gtthread_t *thread, void *(*start_routine)(void *), void *arg)
{
  thread = (gtthread_t*)malloc(sizeof(gtthread_t));
  thread->threadID = threadCtr;
  thread->state = READY;

  thread->runfunction = start_routine;
  thread->arguments = arg;

 //TODO: Store a pointer to the currently running thread
 //thread->parent = 

  thread->childthreads = NULL;

  //Set context
  getcontext(&(thread->context));
  thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
  thread->context.uc_stack.ss_size = STACK_SIZE;
  thread->context.uc_stack.ss_flags = 0;
  thread->context.uc_link = NULL;

  //TODO: Check on number of arguments
  makecontext(&(thread->context), (void(*)())start_routine, 1, arg);

  //Add thread to ready Queue
  addThreadToQueue(&readyQueue, thread);

  //add thread to list of threads
  appendThread(&listOfThreads, thread);
}


/**
 * Join a child thread
 */

int  gtthread_join(gtthread_t thread, void **status)
{
    //Check if thread exists
    gtthread_t *runningThread, *nextThread;

    if(!checkIfThreadExists(thread, listOfThreads))
    {
        fprintf(stderr, "Invalid Thread");
        exit(1);
    } 

    //Get RUnning thread
    runningThread = getRunningThread();  

    if(runningThread == NULL)
    {
        fprintf(stderr, "An error occurred while obtaining the currently executing thread");
        exit(1);
    }

    //Check if thread is a child of the invoking thread
    if(thread.parent->threadID != runningThread->threadID)
    {
        fprintf(stderr, "The thread to be joined is not a child of the currently executing thread");
        exit(1);

    }

    //Add child thread to blocking threads of parent
    appendThread(&(runningThread->blockingThreads), &thread);
    runningThread->state = WAITING;

    //Obtain the first thread from the queue
    nextThread = removeThreadFromQueue(&readyQueue);
    nextThread->state = RUNNING;

    //Swap the context
    swapcontext(&runningThread->context, &nextThread->context);
}

/**
 * Get Running thread
 */
gtthread_t* getRunningThread()
{
  gtthread_node* ptr = listOfThreads;
  gtthread_t* thread;

  while(ptr != NULL)
  {
      if(ptr->thread->state == RUNNING)
      {
          return ptr->thread; 
      }

      ptr = ptr->link;
  }

  return NULL;
}



int main()
{
    return 0;
}

