/******************************************************************************
 *
 *  File Name........: mythread.h
 *
 *  Description......:
 *
 *  Created by vin on 11/21/06
 *
 *  Revision History.:
 *
 *
 *****************************************************************************/

#include "mythread.h"
#include "Queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>




// ****** THREAD OPERATIONS ******


// Create a new thread.
/*
 * Does the following:
 * -> Increment thread counter
 * -> Create a new ThreadStruct variable and initialize its data
 * -> Add the new thread to the ready queue
 * -> Add the new thread the the AllThreads queue
 * -> Return the thread id of the created thread
 */
MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
    threadCounter++;  //Total number of threads so far

    //Initializing the new thread
    ThreadStruct *newthread = (ThreadStruct*) malloc(sizeof(ThreadStruct));
    char new_stack[16384];
    newthread -> start_funct = start_funct;
    newthread -> args = args;
    newthread -> tid = threadCounter;
    newthread -> state = THREAD_READY;
    newthread -> parent = getRunningThread(); //The thread that is currently in running state is its parent
    newthread -> blockingChildren = NULL;
    getcontext(&(newthread->threadctx));
    newthread->threadctx.uc_stack.ss_sp = malloc(1024*16);
    newthread->threadctx.uc_stack.ss_size = 1024*16;
    newthread->threadctx.uc_link=NULL;
    makecontext(&(newthread->threadctx),(void(*)(void))start_funct,1,args);

    //Add the new thread to the ready queue
    addToQueue(readyQ, newthread);

    //Add the new thread to the AllThreads list
    if(AllThreads == NULL)
    {
    	printf("\n MyThreadCreate: Internal warning: AllThreads is null. Output may not be accurate");
        AllThreads = (ThreadList*) malloc(sizeof(ThreadList));
        AllThreads -> thread = newthread;
        AllThreads -> next = NULL;
    }
    else
    {
        ThreadList *newNode = (ThreadList*) malloc(sizeof(ThreadList));
        newNode->thread = newthread;
        newNode->next = AllThreads;
        AllThreads = newNode;
    }


    return (MyThread)(newthread -> tid);
}


// Yield invoking thread
/*
 * Does the following:
 * -> Get the running thread, change its state and put it in ready queue
 * -> Take the thread to the front of ready queue, change its state
 * -> Perform swapcontext between the two threads
 */
void MyThreadYield(void)
{
    //Get running thread
    ThreadStruct *currentThread = getRunningThread();

    if(currentThread==NULL)
    {
    	currentThread=runningThread;
    	if(currentThread==NULL)
    	{
    		printf("\n MyThreadYield: Fatal error! Running thread not found");
    		exit(-1);
    	}
    }

    //change its state
    currentThread->state = THREAD_READY;
    //Put it in ready queue
    addToQueue(readyQ,currentThread);

    //Get the next thread from queue
    ThreadStruct *nextThread=NULL;
        nextThread = removeFromQueue(readyQ);
        if(nextThread==NULL)
        {
        	printf("\n MyThreadYield: Fatal error! Failed to fetch from queue!");
        	exit(-1);
        }
        nextThread -> state = THREAD_RUNNING;
        runningThread=nextThread;


	//Get the last element of queue
    queue *tempq=readyQ;
    while(tempq->link!=NULL)
    {
    	tempq=tempq->link;
    }

    //Swap the context
    swapcontext(&(tempq->thread)->threadctx, &nextThread->threadctx);
}


// Join with a child thread
/*
 * Does the following:
 * -> Check if the child is not an immediate child of parent
 * -> Check if the child thread has finished executing
 * -> If not, attach it to the running thread's blockingChildren list
 * -> Change the state of running thread to THREAD_BLOCKED
 * -> Remove a thread from head of ready queue
 * -> Set the state of the new thread to running
 * -> Perform swapcontext
 */
int MyThreadJoin(MyThread thread)
{
	int result = 0;
	int childid = (int) thread;
	ThreadStruct *child = NULL;
	child = searchThreadList(AllThreads,childid);
	if(child != NULL)
	{
		//printf("\n MyThreadJoin: child is not null. Its id is %d", child->tid);
	}
	else
	{
		printf("\n MyThreadJoin: Child thread does not exist");
		return -1;
	}

	ThreadStruct *parent = getRunningThread();
	if(parent != NULL)
	{
		//printf("\n MyThreadJoin: parent is not null. Its id is %d", parent->tid);
	}
	else if(parent==NULL)
	{
		parent=runningThread;
		if(parent==NULL)
		{
			printf("\n MyThreadJoin: Running thread not found! Fatal error");
			exit(-1);
		}
	}

	//Check if the child is not an immediate child of parent
	if(child->parent->tid != parent -> tid)
	{
		printf("\n MyThreadJoin: The two threads are not parent and child");
		result = -1;
	}
	else
	{

		//Check if the child thread has finished executing
		if(child -> state == 3)
		{
			result = 0;
		}
		else
		{
			//Attach the child to parent's blockingChildren list
			ThreadList *newNode = (ThreadList*) malloc(sizeof(ThreadList));
			newNode->thread = child;
			if(parent->blockingChildren == NULL)
			{
				newNode->next = NULL;
				parent->blockingChildren = newNode;
			}
			else
			{
				newNode->next = parent->blockingChildren;
				parent->blockingChildren = newNode;
			}


			//Change the state of parent to THREAD_BLOCKED
			parent -> state = THREAD_BLOCKED;

			//Remove a thread from head of ready queue
			ThreadStruct *nextThread = removeFromQueue(readyQ);

			//Set the state of nextThread to running
			nextThread->state = THREAD_RUNNING;
			runningThread=nextThread;

			//Perform the swapcontext
			swapcontext(&parent->threadctx, &nextThread->threadctx);
		}
	}

	return result;

}


// Join with all children
/*
 * Does the following:
 * -> Identify all the children of the running thread
 * -> Attach them to the running thread's blockingChildren ThreadList
 * -> Change the status of the running thread to THREAD_BLOCKED
 * -> Attach the running thread to the BlockedThreads ThreadList
 * -> Remove a thread from the head of ready queue
 * -> Set the state of the new thread to running
 * -> Perform swapcontext- save in appropriate thread
 */
void MyThreadJoinAll()
{
    ThreadStruct *parentThread = getRunningThread();
    int isParentBlocked=0;

    if(parentThread==NULL)
    {
    	parentThread=runningThread;
    	if(parentThread==NULL)
    	{
    		printf("\n MyThreadJoinAll: Fatal error! Running thread not found! Exiting");
    		exit(-1);
    	}
    }

    parentThread -> blockingChildren = NULL;  //To avoid multiple entries in parent's blockingChildren


    /*Identify the children. Remove this after making sure it works fine
    ThreadList* tempList=AllThreads;
    printf("\n MyThreadJoinAll: Children of the current thread are: ");
    while(tempList!=NULL)
    {
    	if(tempList->thread->parent!=NULL)
    	if(tempList->thread->parent->tid==parentThread->tid)
    		printf("\t %d",tempList->thread->tid);
       	tempList=tempList->next;
    }*/


    //Attach the children to parent's blockingChildren list
    ThreadList *temp;
    temp = AllThreads;
    while(temp != NULL)
    {
        ThreadStruct *tempThread;
        tempThread = temp -> thread;

        if(tempThread==NULL)
        {
        	printf("\n MyThreadJoinAll: Fatal error! thread not found!");
        	exit(-1);
        }

        if(tempThread->tid == 1)	//Don't bother the mainthread
        {
        	temp = temp->next;
        	continue;
        }

        if(tempThread->parent->tid == parentThread->tid && tempThread->state!=3)
        {
            if(parentThread->blockingChildren == NULL)
            {
                ThreadList *bc = (ThreadList*) malloc(sizeof(ThreadList));
                bc->thread = tempThread;
                bc->next = NULL;
                parentThread->blockingChildren = bc;
                isParentBlocked=1;
            }
            else
            {
                 ThreadList *newNode = (ThreadList*) malloc(sizeof(ThreadList));
                 newNode->thread = tempThread;
                 newNode->next = parentThread->blockingChildren;
                 parentThread->blockingChildren = newNode;
                 isParentBlocked=1;
             }
        }


        temp = temp -> next;
    }


    if(isParentBlocked==1)
    {
    //Change the thread's state to blocked
    parentThread->state = THREAD_BLOCKED;

    //Remove the thread from ready queue
    ThreadStruct *newThread=NULL;
    newThread = removeFromQueue(readyQ);

    //Set the state of new thread to running
    newThread->state = THREAD_RUNNING;
    runningThread=newThread;


    //Swap the context
    swapcontext(&parentThread->threadctx,&newThread->threadctx);
    }

}


// Terminate invoking thread
/*
 * Does the following:
 * -> Get the running thread, set its state to finished
 * -> Check it's parent's blockingChildren list and remove its entry from it.
 * -> If after removing, the list is empty, set its parent's state to ready and put it in ready queue
 * -> If the queue is not empty take a thread from ready queue, set its state to running
 * -> If the queue is empty and if blocked queue is empty too, then swapcontext with main.
 *
 */
void MyThreadExit(void)
{
	ThreadStruct *invokingThread = getRunningThread();
	ThreadList *tempList;
	if(invokingThread == NULL)
	{
		invokingThread=runningThread;
		if(invokingThread==NULL)
		{
			printf("\n MyThreadExit: fatal error! No running thread found! Exiting");
			exit(-1);
		}
	}

	//set its state to finished
	invokingThread->state = THREAD_FINISHED;


	//If all the threads have finished, go back to main
	tempList=AllThreads;
	int finished=1;
	while(tempList!=NULL)
	{
		if(tempList->thread->state!=3)
		{
			finished=0;
		}
		tempList=tempList->next;
	}
	if(finished==1)
		swapcontext(&invokingThread->threadctx,&uctx_main);



	//check its parents blocking children list
	ThreadStruct *parent = invokingThread->parent;

	//For mainthread...
	if(invokingThread->tid==1)
	{
		if(readyQ==NULL)
		{
			swapcontext(&invokingThread->threadctx,&uctx_main);
		}
		if(readyQ!=NULL)
		{
			ThreadStruct *nextThread = removeFromQueue(readyQ);
			if(nextThread==NULL)
			{
				printf("\n MyThreadExit: Fatal error. Thread from queue is null");
				exit(-1);
			}
			nextThread->state = THREAD_RUNNING;
			runningThread=nextThread;
			swapcontext(&invokingThread->threadctx,&nextThread->threadctx);
		}
	}


	//Avoiding the endless if statements that follows.. Yeah it's crazy but lets see if it works
		if(parent->blockingChildren==NULL)
		{
			if(parent->state!=3)
			{
				addToQueue(readyQ,parent);
				parent->state=0;
			}

			if(readyQ != NULL)
			{
				ThreadStruct *nextThread = removeFromQueue(readyQ);
				nextThread->state = THREAD_RUNNING;
				runningThread=nextThread;
				swapcontext(&invokingThread->threadctx,&nextThread->threadctx);
			}
			else
				swapcontext(&invokingThread->threadctx,&uctx_main);
		}


		if(parent->blockingChildren != NULL)
		{
			//Remove the child from parent's blockingChildren ThreadList
			if(parent->blockingChildren->next == NULL)	//If parent's blockingChildren has only one thread
			{
				if(parent->blockingChildren->thread->tid == invokingThread->tid)
				{
					parent->blockingChildren = NULL;
					searchThreadList(AllThreads,parent->tid)->blockingChildren = NULL;
				}
			}
			else if(parent->blockingChildren->thread->tid==invokingThread->tid)	//If the first thread is the one to remove
			{
				parent->blockingChildren=parent->blockingChildren->next;
			}
			else	//If the list has more than one thread
			{
				ThreadList *temp = parent->blockingChildren;
				while(temp->next != NULL)   //Experiment: put temp->next inside while
				{
				     ThreadList *nextToTemp;
				     nextToTemp = temp -> next;
				     if(nextToTemp->thread->tid == invokingThread->tid)
				     {
				          temp -> next = nextToTemp -> next;
				          break;
				     }
				     temp = temp -> next;
				 }
			}
		}

		/*Checking parent's blockingChildren. Delete this if it works fine..
		tempList=AllThreads;
		printf("\n MyThreadExit: After cleanup, the threads in blocking children are: ");
		while(tempList!=NULL)
		{
			if(tempList->thread->tid==parent->tid)
			{
			   ThreadList *bc=tempList->thread->blockingChildren;
			   while(bc!=NULL)
			   {
				   printf("%d\t",bc->thread->tid);
			    	bc=bc->next;
			    }
			}
			tempList=tempList->next;
		}*/


		//If the parent is free, then change its state to ready and put it in ready queue
		if(parent->blockingChildren == NULL)
		{
			parent->state = THREAD_READY;
			addToQueue(readyQ,parent);
		}
		if(readyQ!=NULL)
		{
			ThreadStruct *nextThread = removeFromQueue(readyQ);
			nextThread->state = THREAD_RUNNING;
			runningThread=nextThread;
			swapcontext(&invokingThread->threadctx,&nextThread->threadctx);
		}
		else
		{
			swapcontext(&invokingThread->threadctx,&uctx_main);
		}
}



// ****** SEMAPHORE OPERATIONS ******
// Create a semaphore
MySemaphore MySemaphoreInit(int initialValue)
{
	semaphore *newsem = (semaphore*) malloc(sizeof(semaphore));
	newsem->value=initialValue;
	return newsem;
}


// Signal a semaphore
void MySemaphoreSignal(MySemaphore sem)
{
	semaphore *sema=(semaphore*) sem;
	if(sem==NULL)
	{
		printf("\n MySemaphoreSignal: Fatal error. User program tries to access a non-existent semaphore");
		exit(-1);
	}
	sema->value++;

	//Wake up a thread that is waiting
	if(sema->value<=0)
	{
		if(sema->waitingThreads==NULL)
		{
			return;
		}
		ThreadStruct *wakeupThread = sema->waitingThreads->thread;
		wakeupThread->state = THREAD_READY;
		addToQueue(readyQ,wakeupThread);
		if(sema->waitingThreads->next == NULL)
			sema->waitingThreads = NULL;
		else
			sema->waitingThreads = sema->waitingThreads->next;
	}
}


// Wait on a semaphore
void MySemaphoreWait(MySemaphore sem)
{
	semaphore *sema=(semaphore*)sem;
	if(sem==NULL)
	{
		printf("\n MySemaphoreSignal: Fatal error. User program tries to access a non-existent semaphore");
		exit(-1);
	}
	ThreadStruct* currentThread = getRunningThread();
	sema->value=sema->value-1;

	if(sema->value<0)
	{
		//Block the current process and execute the next process from readyQ
		currentThread->state=THREAD_WAITING;

		//Add the process to semaphore's waitingthreads list
		ThreadList *waitinglistnode = (ThreadList*) malloc(sizeof(ThreadList));
		waitinglistnode->thread=currentThread;
		if(sema->waitingThreads==NULL)
		{
			waitinglistnode->next=NULL;
			sema->waitingThreads=waitinglistnode;
		}
		else
		{
			waitinglistnode->next=sema->waitingThreads;
			sema->waitingThreads=waitinglistnode;
		}


		ThreadStruct *nextThread=removeFromQueue(readyQ);
		if(nextThread==NULL)
		{
			printf("\n MySemaphoreWait: Fatal error. No threads in ready queue. Exiting..");
			exit(-1);
		}
		nextThread->state=THREAD_RUNNING;
		runningThread=nextThread;
		swapcontext(&currentThread->threadctx,&nextThread->threadctx);
	}
}


// Destroy on a semaphore
int MySemaphoreDestroy(MySemaphore sem)
{
	semaphore *sema = (semaphore*) sem;
	if(sema->waitingThreads == NULL)
	{
		free(sema);
		sema=NULL;
		return 0;
	}
	else
	{
		printf("\n MySemaphoreDestroy: Can't destroy semaphore. One or more threads are waiting for it");
		return -1;
	}
}



// ****** CALLS ONLY FOR UNIX PROCESS ******

// Create the "main" thread
/*
 * Does the following:
 * -> Declare/ initialize the variables
 * -> Copy the current context and create the main thread (root of the tree)
 * -> Put the main thread in the ready queue and allthreads ThreadList
 */
void MyThreadInit(void(*start_funct)(void *), void *args)
{
    readyQ = NULL;
    AllThreads = NULL;
    BlockedThreads = NULL;
    threadCounter++;  //Total number of threads so far

    //Initializing the main thread
    ThreadStruct *mainthread = (ThreadStruct*) malloc(sizeof(ThreadStruct));
    char main_stack[16384];
    mainthread -> start_funct = start_funct;
    mainthread -> args = args;
    mainthread -> tid = threadCounter;
    mainthread -> state = THREAD_READY;
    mainthread -> parent = NULL;
    mainthread -> blockingChildren = NULL;
    //Important to call getcontext before makecontext. Wondering why this is needed..
    getcontext(&(mainthread->threadctx));
    mainthread->threadctx.uc_stack.ss_sp = malloc(1024*16);
    mainthread->threadctx.uc_stack.ss_size = 1024*16;
    mainthread->threadctx.uc_link=NULL;
    makecontext(&(mainthread->threadctx),(void(*)(void))start_funct,1,args);

    //Add the main thread to the ready queue
    addToQueue(readyQ, mainthread);

    //Add the main thread to the AllThreads list
    AllThreads = (ThreadList*) malloc(sizeof(ThreadList));
    AllThreads -> thread = mainthread;
    AllThreads -> next = NULL;

}


// Start running the mainthread (i.e) the first thread that is created by main()
/*
 * Does the following:
 * -> Remove the mainthread from the ready queue and start executing it
 */
void MyThreadRun(void)
{
    //Remove the thread from ready queue
    ThreadStruct *mainthread = (ThreadStruct*) malloc(sizeof(ThreadStruct));
    mainthread = removeFromQueue(readyQ);

    if(mainthread != NULL)
    {
        mainthread -> state = THREAD_RUNNING;
        runningThread=mainthread;
        swapcontext(&uctx_main, &(mainthread -> threadctx));
    }
}

