//Author: Akshata Rao

#ifndef GTTHREAD_TYPES_H
#define GTTHREAD_TYPES_H

#include<ucontext.h>

/**
 * Thread State
 */
typedef enum 
{
  NEW,  
  RUNNING,
  WAITING,
  READY,
  TERMINATED  
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
  
  //Pointer to function that has thread functionality
  void* (*runfunction)(void *);   
  
  //Values of arguments to the thread run function
  void *arguments;

  struct thread_node* blockingThreads;

}gtthread_t;


typedef struct thread_node
{
  gtthread_t *thread;
  struct thread_node *link;

}gtthread_node;

#endif
