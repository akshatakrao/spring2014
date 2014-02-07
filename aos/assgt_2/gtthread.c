#include<stdio.h>
#include<gtthread.h>
#include<gtthread_queue.h>
#include<gtthread_list.h>
#include<ucontext.h>
#include<stdlib.h>
#include<string.h>
#include<gtthread_test.h>
#include<gtthread_mutex_list.h>

/**
 * Sets context to Main Context
 */
void mainThreadContextSwitcher()
{   
    setcontext(&mainOrigContext);
}

int entered = 0;
gtthread_t* currentThread = NULL;

/*
 *
 *Init GTThreads
 */
void gtthread_init(long period)
{   

    
   //Check for first entry 
   if(entered == 0)
   {
   //   fprintf(stderr, "\nLOG: Entered");
      entered = 1;
   }
   else //Error handling
   {
     // fprintf(stderr, "\nLOG: Tried to enter again");  
      return;
   }

   //Create Debug File
   createDebugFile();

   //Set Quantum to Period
   quantum_size = period;
   //fprintf(stddebug,"\nLOG: Set Quantum to %ld", period);

	 //Initialize Ready queue
   readyQueue = (gtthread_queue*)malloc(sizeof(gtthread_queue));
   initQueue(&readyQueue); 
   //fprintf(stddebug, "\nLOG: Initialized Ready Queue"); 
    
	//Initialize the scheduler

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
  sigaction(SIGVTALRM, &schedulerHandler, NULL);
  fprintf(stddebug, "\nLOG: Set Scheduler Handler");

  //Insert calling function to the scheduler Queue
  if(getcontext(&mainOrigContext) != 0)
  {
      fprintf(stderr, "\nAn error occurred while setting context");
      exit(1);
  }
  fprintf(stddebug, "\nLOG: Set Calling Function in Main Context");
  
  //START: make main function as gtthread
  mainContext.uc_stack.ss_sp = malloc(STACK_SIZE);
  mainContext.uc_stack.ss_size = STACK_SIZE;
  mainContext.uc_stack.ss_flags = 0;
  mainContext.uc_link = NULL;

  //fprintf(stddebug, "\nLOG: Allocate Main Context");

  mainThread = (gtthread_t*)malloc(sizeof(gtthread_t));
  mainThread->threadID = threadCtr++;
  mainThread->state = RUNNING;
  currentThread = mainThread; 
  //fprintf(stddebug, "\nLOG: Create Main Thread: %ld", mainThread->threadID);

  //Add Main thread to ready queue
  addThreadToQueue(&readyQueue,mainThread);
  fprintf(stddebug, "\nLOG: Added Main Thread to List and Queue");
  getcontext(&(mainThread->context));
  //Set timer for scheduler
  setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
  schedulerFunction();
  //END:

  fprintf(stddebug, "\nLOG: Completed Initialization");
  
}	

/**
 * Wrapper Funnction around a thread's function call
 */
void wrapperFunction(void*(*start_routine)(void*), void* arg)
{
    void* retValue;
    gtthread_t* runningThread = getRunningThread();

    fprintf(stddebug, "\nLOG: In wrapper function %d", (int)arg);
    retValue = start_routine(arg);

  //  fprintf(stddebug, "\nCompleted calling function");

    runningThread = getRunningThread();
//    fprintf(stddebug, "\nReturn Value is %d", (int) retValue);

    //Exit with function return value
    gtthread_exit(retValue);

    setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
    schedulerFunction();//Fail safe for the scheduler

}



/**
 * Create a thread
 */
int  gtthread_create(gtthread_t *thread, void *(*start_routine)(void *), void *arg)
{
//  fprintf(stddebug, "\nLOG: Creating Thread: %d", threadCtr);
  thread->threadID = threadCtr;

  thread->state = READY;
  threadCtr++;
  
  thread->runfunction = start_routine;
  thread->arguments = arg;

  //Store a pointer to the currently running thread
  thread->parent = getRunningThread();

  thread->childthreads = NULL;
  thread->blockingThreads = NULL;
  thread->ownedMutexes = NULL;

  //Create thread context structure
  getcontext(&(thread->context));
  thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
  thread->context.uc_stack.ss_size = STACK_SIZE;
  thread->context.uc_stack.ss_flags = 0;
  thread->context.uc_link = NULL;


  makecontext(&(thread->context), (void (*)(void))&wrapperFunction, 2, start_routine, arg);

  //Add thread to ready Queue
  addThreadToQueue(&readyQueue, thread);
}

/**
 * Get Currently Running thread
 */
gtthread_t* getRunningThread()
{
  gtthread_node* ptr = readyQueue->front;
  gtthread_t* thread = NULL;


  do
  {     
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

    schedulerFunction();
  }while(thread == NULL);


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

int main2();

int main2()
{
  test_join();  
}


/**
 * Scheduler Function
 */
void schedulerFunction()
{
    gtthread_t* frontThread, *selectedThread = NULL, *prevThread;
    gtthread_node* blockingThreads;  

    //Display the state of the Ready queue (Thread ID, Thread State) Front-> Back
    //displayQueue(readyQueue);

    //Switch the currently running thread's state to Ready
    if(currentThread->state == RUNNING)
        currentThread->state = READY;

    int count = 0;
    //fprintf(stddebug, "\nLOG: Ready Queue: %d", readyQueue->count);

    //Iterate through the threads in the queue
    while(count < readyQueue->count)
    {     
      frontThread = removeThreadFromQueue(&readyQueue);

    //  fprintf(stddebug, "\nFront Thread %ld: ", frontThread->threadID);

      //If the thread is suspended
      if(frontThread->state == WAITING)
      {
          blockingThreads = frontThread->blockingThreads;

          int blockingThreadsCompleted = 1;

          //Check if all the blocking threads have completed
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
      }//Else, select the next ready thread
      else if(frontThread->state == READY)
      {
          selectedThread = frontThread;
      }

      addThreadToQueue(&readyQueue, frontThread);
      
      count++;
      if(selectedThread != NULL)
          break;
    }


    if(selectedThread == NULL)
    {
           //Run through the threads till you find the first waiting Thread

          count = 0;

          while(count < readyQueue->count)
          {
              frontThread = removeThreadFromQueue(&readyQueue);
               
              if(frontThread->state == WAITING)
              {
                  selectedThread = frontThread;
                  break;      
              }

              count++;
          }

          if(selectedThread == NULL)
          {
              fprintf(stddebug, "\nAll threads terminated or canceled");
              return;
          }


    }
    selectedThread->state = RUNNING;
    prevThread = currentThread;
    currentThread = selectedThread;
   // fprintf(stddebug, "\nSetting context to Thread ID: %ld" ,selectedThread->threadID);

    setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
    swapcontext(&(prevThread->context),&(selectedThread->context));
}

/**
 * Get The Thread State
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
 Exit the Thread
 */
void gtthread_exit(void *retval)
{
    gtthread_t* thread = getRunningThread();
    gtthread_node* ptr = readyQueue->front;
    gtthread_mutex_node* mutexList = thread->ownedMutexes;

    //fprintf(stddebug, "\nReturn Value Received for thread %ld is %d", thread->threadID, (int)retval);
    thread->returnValue = retval;

    //fprintf(stddebug, "\nReturn Value Saved is %d", (int)thread->returnValue);
    thread->state = TERMINATED;

    //Giving up ownership of all mutexes
    while(mutexList != NULL)
    {
        if(mutexList->mutex->threadID == thread->threadID)
        {
            gtthread_mutex_unlock(mutexList->mutex);
        }
        mutexList = mutexList->link;
    }

    //Call the schedulerFunction 
    schedulerFunction();

}

/**
 * GTThread Yield
 */
int gtthread_yield(void)
{
    setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
    schedulerFunction(); 
    return 0;
}

/**
 * Thread Cancel
 */
int gtthread_cancel(gtthread_t thread)
{
  int dummyValue = CANCELLED;
  gtthread_t* threadptr;
  gtthread_mutex_node* mutexList = thread.ownedMutexes;

  threadptr = getThreadByID(thread.threadID);
  threadptr->state = TERMINATED;
  threadptr->returnValue = dummyValue;

  while(mutexList != NULL)
  {
      if(mutexList->mutex->threadID == thread.threadID)
      {
        gtthread_mutex_unlock(mutexList->mutex);
      }
      mutexList = mutexList->link;
  }
}

/**
 * Obtain the thread structure pointer using the thread ID
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
 * Initialize the mutex
 */
int  gtthread_mutex_init(gtthread_mutex_t *mutex)
{
    if(mutex == NULL)
    {
        fprintf(stddebug,"\nLOG: Cannot pass null parameter to initialization of mutex");
        return 1;
    }
    else
    {
       mutex->mutexID = mutexCtr++; 
       mutex->threadID = INIT_THREAD_ID;
    }
}

/**
 * Checks if lock is available
 * If yes, return 1
 * Else 0
 */
int gtthread_mutex_trylock(gtthread_mutex_t* mutex)
{
    int available = 0;
    sigset_t newset;

    sigemptyset(&newset);
    sigaddset(&newset, SIGVTALRM);
    sigprocmask(SIG_SETMASK, &newset, NULL);

    if(mutex->threadID == INIT_THREAD_ID)
      return 1;
    else
      return 0;

    sigemptyset(&newset);
    sigprocmask(SIG_SETMASK, &newset, NULL);
}


/**
 * Lock the Thread Mutex
 */
int  gtthread_mutex_lock(gtthread_mutex_t *mutex)
{
   //Signaling mask
    gtthread_t* thread;
    int returnValue, yield = 0;
    sigset_t newset;

    sigemptyset(&newset);
    sigaddset(&newset, SIGVTALRM);
    sigprocmask(SIG_SETMASK, &newset, NULL);

    thread = getRunningThread();
    if(mutex == NULL)
    {
        fprintf(stddebug,"\nLOG: Cannot pass null parameter to lock\n");
        returnValue  = 1;
    }
    else if(mutex->threadID == thread->threadID)
    {
      //fprintf(stddebug, "\nLOG: Mutex %d already acquired previously by calling thread\n", mutex->mutexID);
      returnValue = 0;
    }
    else if (mutex->threadID > 0)
    {
       //fprintf(stddebug, "\nLOG: Mutex %d already acquired by thread %ld", mutex->mutexID, thread->threadID);

       yield = 1;

       while(1)
       {
           if(mutex->threadID < 0)
           {
               break;
           }
           else
           {
               //fprintf(stddebug, "\nLOG: Mutex held, yielding thread %ld", thread->threadID);
               sigemptyset(&newset);
               sigprocmask(SIG_SETMASK, &newset, NULL);
               gtthread_yield();
           }

        sigaddset(&newset, SIGVTALRM);
        sigprocmask(SIG_SETMASK, &newset, NULL);
        
       }

       //fprintf(stddebug, "\nLOG: Mutex %d being acquired by thread %ld", mutex->mutexID, thread->threadID);
       
        appendMutex(&(thread->ownedMutexes), mutex);
        mutex->threadID = thread->threadID;
        returnValue = 0;

    }
    else
    { 
       //fprintf(stddebug, "\nLOG: Mutex %d being acquired by thread %ld", mutex->mutexID, thread->threadID);
       
       appendMutex(&(thread->ownedMutexes), mutex);
       mutex->threadID =  thread->threadID;
       returnValue = 0;
    } 
   
    sigemptyset(&newset);
    sigprocmask(SIG_SETMASK, &newset, NULL);

    return returnValue;
   //Signaling mask ends
}


/**
 * Unlock the Thread Mutex
 */
int  gtthread_mutex_unlock(gtthread_mutex_t *mutex)
{
    gtthread_t* thread = getRunningThread();
    int returnValue;
    sigset_t newset;

    sigemptyset(&newset);
    sigaddset(&newset, SIGVTALRM);
    sigprocmask(SIG_SETMASK, &newset, NULL);

    //signal starts here
    if(mutex == NULL)
    {
        fprintf(stddebug,"\nLOG: Cannot pass null parameter to lock");
        returnValue  = 1;
    }
    else if(thread->threadID != mutex->threadID)
    {
        //fprintf(stddebug, "\nLOG:Thread %ld trying to unlock a mutex %d it does not own", thread->threadID, mutex->mutexID);
        returnValue = 1;
    }
    else if(thread->threadID == mutex->threadID)
    {
        //fprintf(stddebug, "\nLOG: Thread %ld unlocking a mutex %d that it owns", thread->threadID, mutex->mutexID);
        mutex->threadID = INIT_THREAD_ID;
        //deleteMutex(&(thread->ownedMutexes), mutex->mutexID); 
    }

    sigemptyset(&newset);
    sigprocmask(SIG_SETMASK, &newset,NULL);  
    //signal ends here
}

/**
 * Thread joineeThread
 *
 */
int gtthread_join(gtthread_t thread, void **status)
{
    gtthread_t *runningThread, *joineeThread = getThreadByID(thread.threadID);


    if(joineeThread == NULL || joineeThread->state == TERMINATED || joineeThread->state == CANCELLED)
    {
    //    fprintf(stderr, "\nERROR: Joinee Thread %ld ceases to exist. Invalid attempt to join on already terminated thread\n", thread.threadID);
        
          if(status != NULL && joineeThread != NULL)
            *status = (joineeThread)->returnValue;
        return 1;
    }

    runningThread = getRunningThread();

    runningThread->state = WAITING;

    appendThread(&(runningThread->blockingThreads), joineeThread);

    setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
    schedulerFunction();

    if(status != NULL) 
    {
        *status = (joineeThread)->returnValue;
    }
//    fprintf(stddebug, "\nThread Join Return Value: %d", (int) joineeThread->returnValue);
    
}

/* TODO:
 *
 * 3. Make File
 * 6. Dining Philosopher
 * 8. Write Up
 *
 * */
