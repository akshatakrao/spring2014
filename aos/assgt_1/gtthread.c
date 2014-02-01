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


/**
 *
 */
int gtthread_yield()
{

    //Get currently running thread
    gtthread_node* runningThread = getRunningThread();
    gtthread_node* nextThread = NULL;

    //Check if running thread exists
    if(runningThread == NULL)
    {
        fprintf(stderr, "No thread found to be running right now!");
        return 1;
    }

   nextThread = removeThreadFromQueue(&readyQueue);

   //If another thread exists in the ready queue, then
   //swap contexts
   
   if(nextThread == NULL)
   {
        fprintf(stderr, "No other thread found in the ready queue");
        return 0;
   }

   runningThread->state = READY;
   addThreadToQueue(runningThread);

   nextThread->state = RUNNING;
   swapcontext(&(runningThread->context), &(nextThread->context));

    //Push thread to back of queue
    //Bring the next thread to front of queue
    //Do a swap of context

   return 0;
}

/**
 *
 */
void gtthread_exit(void* retval)
{
  gtthread_t* runningThread = NULL, *nextThread = NULL;
  gtthread_node* ptrToChildren = NULL, *nextThreadNode = NULL;
  int returnVal = 0;

  //Get Running Thread
  runningThread = getRunningThread();

  if(runningThread == NULL)
  {
      fprintf(stderr, "\nNo running thread was found.");
      exit(1);
  }

  //Change status of running thread
  runningThread->state = TERMINATED;
  ptrToChildren = runningThread->childthreads;

  //Terminate the child threads recursively
  while(ptrToChildren != NULL)
  {
    killChildThreads(ptrToChildren);
    ptrToChildren = ptrToChildren->link;
  }

  nextThreadNode = removeThreadFromQueue(&readyQueue); 

  if(nextThreadNode != NULL)
  {
    nextThread = nextThreadNode->thread;
    swapcontext(&runningThread->context, &nextThread->context);
  }
  
  //TODO: Set return value in retval

  retval = (void *)&returnVal;
}

/**
 * Kill Child Threads
 */
void killChildThreads(gtthread_node* threadNode)
{
    
  gtthread_t* thread = threadNode->thread;  
  gtthread_node* childthreads = thread->childthreads;

  thread->state = TERMINATED;

  while(childthreads != NULL)
  {

      if(childthreads->thread->state != TERMINATED)
            killChildThreads(childthreads);
      childthreads = childthreads->link;
  }
}
  
/**
 * Check if threads are equal
 */     
int  gtthread_equal(gtthread_t t1, gtthread_t t2)
{

    if((t1 != NULL && t2 != NULL) && (t1->threadID == t2->threadID))
    {
       return 1;
    }
}

int main()
{
    return 0;
}

