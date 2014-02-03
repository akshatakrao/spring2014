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
{    setcontext(&mainContext);
}

int entered = 0, counter = 0;
gtthread_t* currentThread = NULL;
/*
 *
 *Init GTThreads
 */
void gtthread_init(long period)
{   
    if(entered == 0)
    {
      fprintf(stderr, "\nLOG: Entered");
     entered = 1;
   }
   else
   {
      fprintf(stderr, "\nLOG: Tried to enter again");  
      return;
   }

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
	timeSlice.it_interval.tv_sec = 0;
  timeSlice.it_interval.tv_usec = (long) period;
  	
	
  //Initialize scheduler signal handler
  //TODO: Check if its PROF alarm
  memset(&schedulerHandler, 0, sizeof(schedulerHandler));
  schedulerHandler.sa_handler=&schedulerFunction;
  sigemptyset(&schedulerHandler.sa_mask);
  sigaction(SIGALRM, &schedulerHandler, NULL);
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
  mainContext.uc_link = NULL;//&schedulerContext;
  makecontext(&mainContext, &mainThreadContextSwitcher, 0);

  fprintf(stddebug, "\nLOG: Allocate Main Context");

  mainThread = (gtthread_t*)malloc(sizeof(gtthread_t));
  mainThread->threadID = threadCtr++;
  mainThread->state = RUNNING;
  mainThread->context = mainContext;
  currentThread = mainThread; 
  fprintf(stddebug, "\nLOG: Create Main Thread: %ld", mainThread->threadID);

  addThreadToQueue(&readyQueue,mainThread);
  appendThread(&listOfThreads, mainThread);
  fprintf(stddebug, "\nLOG: Added Main Thread to List and Queue");

  setitimer(ITIMER_REAL, &timeSlice, NULL);  
  //END:

  counter++;
  fprintf(stddebug, "\nLOG: Counter: %d",counter);

 // while(1);
  //schedulerFunction();
  fprintf(stddebug, "\nLOG: Completed Initialization");
  
}	

/**
 * Wrapper Function
 */
void wrapperFunction(void*(*start_routine)(void*), void* arg)
{
    void* retValue;
    gtthread_t* runningThread = getRunningThread();

    fprintf(stddebug, "\nLOG: In wrapper function");
    retValue = start_routine(arg);

    fprintf(stddebug, "\nCompleted calling function");

    runningThread = getRunningThread();
    gtthread_exit(retValue);
    //runningThread->state = TERMINATED;
    
    //runningThread->returnValue = retValue;
    fprintf(stddebug, "\nLOG: Calling scheduler");

    setitimer(ITIMER_REAL, &timeSlice, NULL);
    
    schedulerFunction();
    //gtthread_cancel(*runningThread);
}



/**
 * Create a thread
 */
int  gtthread_create(gtthread_t *thread, void *(*start_routine)(void *), void *arg)
{
  fprintf(stddebug, "\nLOG: Creating Thread: %d", threadCtr);
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
  thread->context.uc_link = NULL;//&schedulerContext;

  //TODO: Check on number of arguments

  makecontext(&(thread->context), (void (*)(void))&wrapperFunction, 2, start_routine, arg);

  //Add thread to ready Queue
  addThreadToQueue(&readyQueue, thread);

  //add thread to list of threads
  appendThread(&listOfThreads, thread);
}

/**
 * Get Running thread
 */
gtthread_t* getRunningThread()
{
  gtthread_node* ptr = readyQueue->front;
  gtthread_t* thread = NULL;

  while(ptr != NULL)
  {
      //fprintf(stddebug, "\nLOG: GRunning Thread: %ld %d %d", ptr->thread->threadID, ptr->thread->state, RUNNING); 
      if(ptr->thread->state == RUNNING)
      {
        //  fprintf(stddebug, "\nLOG: Running Thread Found: %ld", ptr->thread->threadID);
          thread = ptr->thread; 
      }

      ptr = ptr->link;
  }

  //fprintf(stddebug, "\nLOG: Running Thread Selected %ld", thread->threadID);
  return thread;
}


/**
 * Return Thread Self
 */
gtthread_t gtthread_self(void)
{
    gtthread_t* threadPtr = getRunningThread();

    return *threadPtr;
}

/**
 * Check if threads are equal
 */
int gtthread_equal(gtthread_t t1, gtthread_t t2)
{
  if(t1.threadID == t2.threadID)
      return 1;
  else
      return 0;

}

int main()
{
  test_join();  
//	test_exit();
//test_init();
}

 void printMain();
 struct sigaction handler;  

 int main2()
 {
      timeSlice.it_value.tv_sec = 0;
      timeSlice.it_value.tv_usec = 500000;
      timeSlice.it_interval = timeSlice.it_value;
      setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
  
      memset(&handler, 0 , sizeof(handler));
      handler.sa_handler=printMain;
      sigaction(SIGVTALRM, &handler, NULL) ;
      setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
 
      while(1);
  
  }
  
 void printMain()
{
    printf("In Main");
}


static int firstEntry = 0;

/**
 * Scheduler Function
 */


void schedulerFunction()
{
    gtthread_t* frontThread, *selectedThread = NULL, *prevThread;
    gtthread_node* blockingThreads;  

    displayQueue(readyQueue);

    int a;
    if(currentThread->state == RUNNING)
        currentThread->state = READY;

    int count = 0;

    while(count < readyQueue->count)
    {     
      frontThread = removeThreadFromQueue(&readyQueue);
      
      if(frontThread->state == WAITING)
      {
          blockingThreads = frontThread->blockingThreads;

          int blockingThreadsCompleted = 1;

          while(blockingThreads != NULL)
          {
            gtthread_state blockingThreadState = getThreadState(blockingThreads->thread->threadID);

            if(blockingThreadState != TERMINATED && blockingThreadState != CANCELLED)
            {
                fprintf(stddebug, "\nLOG: Blocked Thread %ld is still blocked on Thread %ld with state %d, %d", frontThread->threadID, blockingThreads->thread->threadID, blockingThreads->thread->state, TERMINATED); 
                blockingThreadsCompleted = 0;
                break; 
            }

            blockingThreads = blockingThreads->link;
          } 

          if(blockingThreadsCompleted)
          {
              fprintf(stddebug, "\nLOG: Block Thread Unblocked : %ld", frontThread->threadID);
              selectedThread = frontThread;
          }
      }
      else if(frontThread->state == READY)
      {
          selectedThread = frontThread;
      }

      addThreadToQueue(&readyQueue, frontThread);
      
      count++;
      if(selectedThread != NULL)
          break;
    }


    selectedThread->state = RUNNING;
    prevThread = currentThread;
    currentThread = selectedThread;
    fprintf(stddebug, "\nSetting context to Thread ID: %ld" ,selectedThread->threadID);
    //setitimer(ITIMER_REAL, &timeSlice, NULL);

    setitimer(ITIMER_REAL, &timeSlice, NULL);
    swapcontext(&(prevThread->context),&(selectedThread->context));
}

/**
 *
 */
gtthread_state getThreadState(long threadID)
{
  gtthread_node* ptr = readyQueue->front;

  while(ptr != NULL)
  {
      if(ptr->thread->threadID == threadID)
      {
          return ptr->thread->state;
      }

      ptr = ptr->link;
  }
}

/*

 */
void gtthread_exit(void *retval)
{
    gtthread_t* thread = getRunningThread();
    gtthread_node* ptr = listOfThreads;

    thread->returnValue = retval;
    thread->state = TERMINATED;

    if(thread->threadID == 0)
    { 
        fprintf(stddebug, "\nLOG: Calling exit on the main function. Kill all child threads");
        
        while(ptr != NULL)
        {
            ptr = listOfThreads;
            
            if(ptr->thread->state != TERMINATED || ptr->thread->state != CANCELLED)
            {
                ptr->thread->state = TERMINATED;
            }  

            ptr = ptr->link;
        }
        
    }
    else
    {
        fprintf(stddebug, "\nLOG: Called Exit on Non-Main. Return value to Joined thread");
    }
    schedulerFunction();

}

/**
 * Thread Cancel
 */
int gtthread_cancel(gtthread_t thread)
{
  int dummyValue = CANCELLED;
  gtthread_exit(&dummyValue);

}

/**
 *
 */
gtthread_t* getThreadByID(long threadID)
{
    gtthread_t* thread = NULL;
    gtthread_node* ptr = readyQueue->front;

    while(ptr != NULL)
    {
        if(ptr->thread->threadID == threadID)
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
int gtthread_join(gtthread_t thread, void **status)
{
    gtthread_t *runningThread, *joineeThread = getThreadByID(thread.threadID);

    if(joineeThread == NULL)
    {
        fprintf(stderr, "\nERROR: Invalid Thread ID to join on: %ld", thread.threadID);
        exit(1);
    }

    runningThread = getRunningThread();

    runningThread->state == WAITING;
    appendThread(&(runningThread->blockingThreads), joineeThread);

     setitimer(ITIMER_REAL, &timeSlice, NULL);
    //schedulerFunction();
}

int gtthread_join2(gtthread_t, void **);
/**
 *Thread Join
 */
int  gtthread_join2(gtthread_t thread, void **status)
{
    gtthread_node* ptr;
    gtthread_t* joinedThread, *runningThread = getRunningThread();

    if(runningThread == NULL)
    {
        fprintf(stddebug, "\nERROR: No Running threads found?!");
        return 0;  
        // exit(1);
    }

    fprintf(stddebug, "\nLOG: Running Thread %ld joining on Thread %ld", runningThread->threadID, thread.threadID);
    runningThread->state = WAITING;
    appendThread(&(runningThread->blockingThreads), &thread);

    ptr = listOfThreads;

    while(ptr != NULL)
    {
        if(ptr->thread->threadID == thread.threadID)
        {
          joinedThread = ptr->thread;
          break;
        }
    }

    while(1)
    {
        if((joinedThread->state == TERMINATED || joinedThread->state == CANCELLED))
        {
            fprintf(stddebug, "\nLOG: Joined Thread %ld terminated ", thread.threadID);
            status = &(joinedThread->returnValue); 
            break;
        }
   }
    //schedulerFunction();
   
    fprintf(stddebug, "\nLOG: Returned from Join"); 
    return 1;//Success 
}

//Fix scheduler - Send non ready and not eligible to be unblocked threads to the end.
//Fix state - Thread State from Running to Ready
//Fix thread join
