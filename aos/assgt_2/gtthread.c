#include<stdio.h>
#include<gtthread.h>
#include<gtthread_queue.h>
#include<gtthread_list.h>
#include<ucontext.h>
#include<stdlib.h>
//#include<gtthread_scheduler.h>
#include<string.h>
#include<gtthread_test.h>

/**
 * Sets context to Main Context
 */
void mainThreadContextSwitcher()
{
    setcontext(&mainContext);
}

int counter = 0;

/*
 *
 *Init GTThreads
 */
void gtthread_init(long period)
{
   createDebugFile();

   quantum_size = period;
   fprintf(stddebug,"\nLOG: Set Quantum to %ld", period);

	//Initialize Ready queue
  readyQueue = (gtthread_queue*)malloc(sizeof(gtthread_queue));
   initQueue(&readyQueue); 
   fprintf(stddebug, "\nLOG: Initialized Ready Queue"); 
    
	//Initialize the scheduler
	if (getcontext(&schedulerContext) == 0) 
	{
		schedulerContext.uc_stack.ss_sp = malloc(STACK_SIZE);
		schedulerContext.uc_stack.ss_size = STACK_SIZE;
		schedulerContext.uc_stack.ss_flags = 0;
		schedulerContext.uc_link = NULL;
    fprintf(stddebug, "\nLOG: Created Scheduler Context");


	} else {
		fprintf(stddebug, "\nAn error occurred while initializing scheduler context");
		exit(1);
	}

  //Initialize timeSlice
  timeSlice.it_value.tv_sec = 0; 
	timeSlice.it_value.tv_usec =(long)period;
	timeSlice.it_interval = timeSlice.it_value;
  	
	
  //Initialize scheduler signal handler
  //TODO: Check if its PROF alarm
  memset(&schedulerHandler, 0, sizeof(schedulerHandler));
  schedulerHandler.sa_handler=&schedulerFunction;
  sigaction(SIGVTALRM, &schedulerHandler, NULL);
  fprintf(stddebug, "\nLOG: Set Scheduler Handler");

  //Insert calling function to the scheduler Queue
  if(getcontext(&mainContext) != 0)
  {
      fprintf(stderr, "\nAn error occurred while setting context");
      exit(1);
  }
  fprintf(stddebug, "\nLOG: Set Calling Function in Main Context");
  
  //START: make main function as gtthread
  mainContext.uc_stack.ss_sp = malloc(STACK_SIZE);
  mainContext.uc_stack.ss_size = STACK_SIZE;
  mainContext.uc_stack.ss_flags = 0;
  mainContext.uc_link = &schedulerContext;
  makecontext(&mainContext, &mainThreadContextSwitcher, 0);

  fprintf(stddebug, "\nLOG: Allocate Main Context");

  mainThread = (gtthread_t*)malloc(sizeof(gtthread_t));
  mainThread->threadID = threadCtr++;
  mainThread->state = READY;
  mainThread->context = mainContext; 
  fprintf(stddebug, "\nLOG: Create Main Thread: %ld", mainThread->threadID);

  addThreadToQueue(&readyQueue,mainThread);
  appendThread(&listOfThreads, mainThread);
  fprintf(stddebug, "\nLOG: Added Main Thread to List and Queue");

  //setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);  
  //END:

  counter++;
  fprintf(stddebug, "\nLOG: Counter: %d",counter);
  schedulerFunction();
  fprintf(stddebug, "\nI came here");
  
}	
	


/**
 * Create a thread
 */
int  gtthread_create(gtthread_t *thread, void *(*start_routine)(void *), void *arg)
{
  thread->threadID = threadCtr;

  thread->state = READY;
  threadCtr++;
  
  thread->runfunction = start_routine;
  thread->arguments = arg;

 //TODO: Store a pointer to the currently running thread
  thread->parent = getRunningThread();

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
  setcontext(&schedulerContext);
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
 * Return Thread Self
 */
gtthread_t gtthread_self(void)
{
    gtthread_t* threadPtr = getRunningThread();

    return *threadPtr;
}


int main()
{
    
	test_init();
}

static int firstEntry = 0;

void schedulerFunction()
{
    int count = 0;
    gtthread_node* currentPtr;
    gtthread_t* temp;

    fprintf(stddebug, "\nLOG: Im in scheduler");

    if(firstEntry == 0)
    {

        firstEntry = 1;
        getcontext(&schedulerContext);
        schedulerContext.uc_stack.ss_sp = malloc(STACK_SIZE);
        schedulerContext.uc_stack.ss_size = STACK_SIZE;
        schedulerContext.uc_stack.ss_flags = 0;
        schedulerContext.uc_link = NULL;
        makecontext(&schedulerContext, &schedulerFunction, 0);
        fprintf(stddebug, "\nLOG: Saved scheduler context");  

    } 

    displayQueue(readyQueue);
    displayList(listOfThreads);
    int test;

    if(readyQueue->count == 1)
    { 
        fprintf(stddebug, "Only 1");
        scanf("%d", &test);
        return;
    }
     
    while(count < readyQueue->count)
    {
        currentPtr = readyQueue->front;
 
        fprintf(stddebug, "\nLOG: Reading front of the queue: Thread ID: %ld", currentPtr->thread->threadID);
        
        //TODO: Check if the thread state can ever be RUNNING
        if(currentPtr != NULL && currentPtr->thread->state == READY)
        {
            fprintf(stddebug, "\nLOG: Swapping scheduler and thread context");
            //setcontext(&(currentPtr->thread->context)); 
            setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);          
            setcontext(&currentPtr->thread->context); 
        }
        else
        {
            fprintf(stderr, "\nLOG: Thread %ld in state %d. Sending to back of Queue", currentPtr->thread->threadID, currentPtr->thread->state); 
            temp = removeThreadFromQueue(&readyQueue);
            addThreadToQueue(&readyQueue, temp);
        }

        //Incrementing count
        count++;

    }
}

