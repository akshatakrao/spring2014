#include<stdio.h>
#include<gtthread.h>
#include<gtthread_queue.h>
#include<gtthread_list.h>
#include<ucontext.h>
#include<stdlib.h>
#include<string.h>
#include<gtthread_test.h>
#include<gtthread_mutex_list.h>


int entered = 0;
gtthread_t* currentThread = NULL;

/*
 * Function: Initialize GTThreads
 * Arg: Time Slice Quantum
 * Returns: None 
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
  memset(&schedulerHandler, 0, sizeof(schedulerHandler));
  schedulerHandler.sa_handler=&schedulerFunction;
  sigemptyset(&schedulerHandler.sa_mask);
  sigaction(SIGVTALRM, &schedulerHandler, NULL);
  //fprintf(stddebug, "\nLOG: Set Scheduler Handler");

  //Insert calling function to the scheduler Queue
  
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
  //fprintf(stddebug, "\nLOG: Added Main Thread to List and Queue");
  getcontext(&(mainThread->context));

  //Set timer for scheduler
  setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
  schedulerFunction();

 // fprintf(stddebug, "\nLOG: Completed Initialization");
  
}	

/**
 * Function: Wrapper Funnction around a thread's function call
 * Arguments: start_routine - Thread Start Routine
 *            arg - arguments
 * Returns: None
 */
void wrapperFunction(void*(*start_routine)(void*), void* arg)
{
    void* retValue;
    gtthread_t* runningThread = getRunningThread();

   // fprintf(stddebug, "\nLOG: In wrapper function %d", (int)arg);
    retValue = start_routine(arg);

    //fprintf(stddebug, "\nCompleted calling function");

    runningThread = getRunningThread();
    //fprintf(stddebug, "\nReturn Value is %d", (int) retValue);

    //Exit with function return value
    gtthread_exit(retValue);

    setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
    //schedulerFunction();//Fail safe for the scheduler

}

/**
 * Function: Creates a thread
 * Arguments: thread - Thread
 *            start_routine - Calling Routine
 *            arg - Arguments to start Routine
 * Returns: 0 upon success
 */
int  gtthread_create(gtthread_t *thread, void *(*start_routine)(void *), void *arg)
{
//  fprintf(stddebug, "\nLOG: Creating Thread: %d", threadCtr);
  thread->threadID = threadCtr;

  //Set thread state to Ready
  thread->state = READY;
  threadCtr++;
 
  //Set Thread Function to Running 
  thread->runfunction = start_routine;
  thread->arguments = arg;

  //Store a pointer to the currently running thread
  thread->parent = getRunningThread();

  //Initialize blocking threads and owned mutexes
  thread->blockingThreads = NULL;
  thread->ownedMutexes = NULL;

  //Create thread context structure
  getcontext(&(thread->context));
  thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
  thread->context.uc_stack.ss_size = STACK_SIZE;
  thread->context.uc_stack.ss_flags = 0;
  thread->context.uc_link = NULL;

  //Make context for the thread 
  makecontext(&(thread->context), (void (*)(void))&wrapperFunction, 2, start_routine, arg);

  //Add thread to ready Queue
  addThreadToQueue(&readyQueue, thread);
}

/**
 * Function: Get currently running thread
 * Arguments: None
 * Returns: Currently Running Thread
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
 * Function: Get Thread structure of calling thread
 * Arguments: None
 * Return: Thread structure
 */
gtthread_t gtthread_self(void)
{
    gtthread_t* threadPtr = getRunningThread();

    return *threadPtr;
}

/**
 * Function: Check if threads are equal
 * Arguments: t1, t2 - Thread structures
 * Return: 1 if the two threads are the same
 */
int gtthread_equal(gtthread_t t1, gtthread_t t2)
{
  if(t1.threadID == t2.threadID)
      return 1;
  else
      return 0;

}

//START: Test Code
int main2();

int main2()
{
  test_join();  
}
//END: Test Code

/**
 * Function: Scheduler Function
 * Arguments: None
 * Return: None
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
                //fprintf(stddebug, "\nLOG: Blocked Thread %ld is still blocked on Thread %ld with state %d, %d", frontThread->threadID, blockingThreads->thread->threadID, blockingThreads->thread->state, TERMINATED); 
                blockingThreadsCompleted = 0;
                break; 
            }

            blockingThreads = blockingThreads->link;
          } 

          //If all blocking threads have completed, unblock the thread
          if(blockingThreadsCompleted)
          {
              //fprintf(stddebug, "\nLOG: Block Thread Unblocked : %ld", frontThread->threadID);
              selectedThread = frontThread;
          }
      }//Else, select the next ready thread
      else if(frontThread->state == READY)
      {
          selectedThread = frontThread;
      }

      //Add the thread back to the ready queue
      addThreadToQueue(&readyQueue, frontThread);
      
      count++;
      if(selectedThread != NULL)
          break;
    }


    //If all threads but one have terminated, and the remaining are awaiting, unblock them
    if(selectedThread == NULL)
    {
           //Run through the threads till you find the first waiting Thread

          count = 0;

          while(count < readyQueue->count)
          {
              frontThread = removeThreadFromQueue(&readyQueue);
              appendThread(&readyQueue, frontThread); 
              
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

    //Set the selected thread to RUNNING 
    selectedThread->state = RUNNING;
    prevThread = currentThread;
    currentThread = selectedThread;
   // fprintf(stddebug, "\nSetting context to Thread ID: %ld" ,selectedThread->threadID);

    //Schedule the timer again
    setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
    swapcontext(&(prevThread->context),&(selectedThread->context));
}

/**
 * Function: Get The Thread State
 * Arguments: Thread ID
 * Return: Get The Thread State
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
  Function: Exit the Thread
  Arguments: Return Value pointer
  Returns: None
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
        if(mutexList->mutex == NULL)
        {
           fprintf(stderr, "Mutex is NULL");
        }

        if(thread == NULL)
        {
            fprintf(stderr, "Thread is NULL");
        }

        if(mutexList->mutex != NULL && mutexList->mutex->threadID == thread->threadID)
        {
            gtthread_mutex_unlock(mutexList->mutex);
        }
        mutexList = mutexList->link;
    }

    //Call the schedulerFunction 
    schedulerFunction();

}

/**
 * Function: Yield to the next Thread
 * Arguments: None
 * Return: Success 
 */
int gtthread_yield(void)
{
    //setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
    schedulerFunction(); 
    return 0;
}

/**
 * Function: Cancel a thread
 * Arguments: thread
 * Return: Success 
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
 * Function: Obtain the thread structure pointer using the thread ID
 * Arguments: Thread ID
 * Return: Thread Pointer
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
 * Function: Initialize the mutex
 * Arguments: Pointer to mutex
 * Returns: 0 for Success, 1 for Failure
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

    return 0;
}

/**
 * Function:Checks if lock is available
 * Arguments: Thread mutex
 * Returns: 1 for available, 0 for failure
 */
int gtthread_mutex_trylock(gtthread_mutex_t* mutex)
{
    int available = 0;
    sigset_t newset;

    //Mask the scheduler signals
    sigemptyset(&newset);
    sigaddset(&newset, SIGVTALRM);
    sigprocmask(SIG_SETMASK, &newset, NULL);

    //Check if mutex is held by any thread
    if(mutex->threadID == INIT_THREAD_ID)
      return 1;
    else
      return 0;

    //Unmask the scheduler signals
    sigemptyset(&newset);
    sigprocmask(SIG_SETMASK, &newset, NULL);
}


/**
 * Function: Lock the Thread Mutex
 * Arguments: Mutex
 * Return: 0 for success, 1 for failure
 */
int  gtthread_mutex_lock(gtthread_mutex_t *mutex)
{
   //Signaling mask
    gtthread_t* thread;
    int returnValue, yield = 0;
    sigset_t newset;

    //Mask the Thread signals
    sigemptyset(&newset);
    sigaddset(&newset, SIGVTALRM);
    sigprocmask(SIG_SETMASK, &newset, NULL);

    thread = getRunningThread();

    //Error handling
    if(mutex == NULL)
    {
        fprintf(stddebug,"\nLOG: Cannot pass null parameter to lock\n");
        returnValue  = 1;
    }
    else if(mutex->threadID == thread->threadID)//Is mutex already held by thread
    {
      //fprintf(stddebug, "\nLOG: Mutex %d already acquired previously by calling thread\n", mutex->mutexID);
      returnValue = 0;
    }
    else if (mutex->threadID > 0)//Is mutex held by another thread
    {
       //fprintf(stddebug, "\nLOG: Mutex %d already acquired by thread %ld", mutex->mutexID, thread->threadID);

       yield = 1;

       //Spin Lock till mutex is released
       while(1)
       {
           if(mutex->threadID < 0)
           {
               break;
           }
           else
           {
               //fprintf(stddebug, "\nLOG: Mutex held, yielding thread %ld", thread->threadID);
               //Unmask the signals
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
  

   //Unmask the signal 
    sigemptyset(&newset);
    sigprocmask(SIG_SETMASK, &newset, NULL);

    return returnValue;
   //Signaling mask ends
}


/**
 * Function: Unlock the Thread Mutex
 * Arguments: Mutex pointer
 * Returns: 0 if success, 1 if failure
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
    else if(thread->threadID != mutex->threadID)//Is thread trying to unlock an unowned mutex
    {
        //fprintf(stddebug, "\nLOG:Thread %ld trying to unlock a mutex %d it does not own", thread->threadID, mutex->mutexID);
        returnValue = 1;
    }
    else if(thread->threadID == mutex->threadID)//Is thread trying to unlock held mutex
    {
        //fprintf(stddebug, "\nLOG: Thread %ld unlocking a mutex %d that it owns", thread->threadID, mutex->mutexID);
        mutex->threadID = INIT_THREAD_ID;
    }

    sigemptyset(&newset);
    sigprocmask(SIG_SETMASK, &newset,NULL);  
    //signal ends here
}

/**
 * Function: Thread attempts to join a thred
 * Arguments: thread - Joinee Thread
 *            status - Return value of joined thread
 * Returns: None
 */
int gtthread_join(gtthread_t thread, void **status)
{
    gtthread_t *runningThread, *joineeThread = getThreadByID(thread.threadID);


    //If Joinee Thread is NULL/Terminated/Cancelled, return
    if(joineeThread == NULL || joineeThread->state == TERMINATED || joineeThread->state == CANCELLED)
    {
    //    fprintf(stderr, "\nERROR: Joinee Thread %ld ceases to exist. Invalid attempt to join on already terminated thread\n", thread.threadID);
        
          if(status != NULL && joineeThread != NULL)
          {
            *status = (joineeThread)->returnValue;
          }

        return 1;
    }

    runningThread = getRunningThread();

    runningThread->state = WAITING;

    //Add joinee thread to blocking threads for the current thread
    appendThread(&(runningThread->blockingThreads), joineeThread);

    //Call scheduler
    //setitimer(ITIMER_VIRTUAL, &timeSlice, NULL);
    schedulerFunction();

    //Send thread return value in joinee thread
    if(status != NULL) 
    {
        *status = (joineeThread)->returnValue;
    }
//    fprintf(stddebug, "\nThread Join Return Value: %d", (int) joineeThread->returnValue);
    
}

/* TODO:
 *
 * 3. Make File
 * 8. Write Up
 * 10. Insert locks in the init and stuff 
 * */
