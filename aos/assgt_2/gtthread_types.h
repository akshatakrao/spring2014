//Author: Akshata Rao

#ifndef GTTHREAD_TYPES_H
#define GTTHREAD_TYPES_H

#include<ucontext.h>

/**
 * Thread State
 */
typedef enum 
{
  RUNNING,
  WAITING,
  READY,
  TERMINATED,
  CANCELLED 
}gtthread_state;

/**
 * Thread Structure
 */
typedef struct gtthread
{
  long threadID; //Thread ID
  gtthread_state state; //Thread State
  ucontext_t context; //Thread Context

  //Thread parent
  struct gtthread* parent;

  //Thread forked Children
  struct thread_node* childthreads;
  struct thread_node* blockingThreads;
  struct thread_mutex_node* ownedMutexes;

  //Pointer to function that has thread functionality
  void* (*runfunction)(void *);   
  
  //Values of arguments to the thread run function
  void *arguments;

  void *returnValue;	
}gtthread_t;

/**
 * Thread Node Structure
 */
typedef struct thread_node
{
  gtthread_t *thread;
  struct thread_node *link;

}gtthread_node;

/**
 * Thread Mutex Structure
 */
typedef struct thread_mutex
{
    int mutexID;
    long threadID;
}gtthread_mutex_t;

typedef struct thread_mutex_node
{
    gtthread_mutex_t* mutex;
    struct thread_mutex_node *link;

}gtthread_mutex_node;

#endif
