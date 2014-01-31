/*
 * File:   ThreadStruct.h
 * Author: aswin
 * Description: This defines the encapsulation of a thread object, implemented by a struct
 *
 * Created on August 23, 2011, 10:28 PM
 */

#include<stdlib.h>
#include <ucontext.h>

#ifndef THREADSPECIFICS_H
#define	THREADSPECIFICS_H

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* THREADSPECIFICS_H */


//Defining the thread states
#define THREAD_READY     0;
#define THREAD_RUNNING   1;
#define THREAD_BLOCKED   2;
#define THREAD_FINISHED  3;
#define THREAD_WAITING   4;

int threadCounter;

typedef struct ThreadStruct
{
    ucontext_t threadctx;                 //Thread context
    void(*start_funct)(void *);         //Start function for the thread
    void *args;                         //Arguments to be passed to start_funct()
    int tid;                            //Thread id
    int state;
    struct ThreadStruct* parent;               //parent thread
    struct ThreadList* blockingChildren;//Threads that are blocking the thread
}ThreadStruct;

ThreadStruct *runningThread;

//ThreadList is a linked list for storing a set of threads
typedef struct ThreadList
{
    struct ThreadStruct *thread;
    struct ThreadList *next;
}ThreadList;

ThreadList *AllThreads, *BlockedThreads;

static ucontext_t uctx_main;	//for storing the context of main() function


/*Operations on ThreadList*/

//Search for a particular thread and return it
ThreadStruct* searchThreadList(ThreadList *list, int thrid);

//Remove a particular thread
void removeFromThreadList(ThreadList *list, ThreadStruct *thr);

//Add a thread at the end
void addToThreadList(ThreadList *list, ThreadStruct *thr);


//Get the thread that is currently running
ThreadStruct* getRunningThread();




/**********SEMAPHORE SPECIFICS*************/
typedef struct semaphore
{
	int value;
	ThreadList *waitingThreads;
}semaphore;
