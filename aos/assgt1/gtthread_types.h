//Author: Akshata Rao

#ifndef GTTHREAD_TYPES_H
#define GTTHREAD_TYPES_H
#endif

//#include<gtthread.h>
#include<gtthread_list.h>
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
typedef struct gtthread_t
{
	long threadID; //Thread ID
	gtthread_state state; //Thread State
  ucontext_t context; //Thread Context

  //Thread parent
  gtthread_t* parent;

  //Thread forked Children
  gtthread_list* childthreads;
  
  //Pointer to function that has thread functionality
  void *runfunction(void *);   
  
  //Values of arguments to the thread run function
  void *arguments;

	
}gtthread_t;


struct gtthread_node
{
  gtthread_t thread;
  gtthread_t *link;

}gtthread_node;


