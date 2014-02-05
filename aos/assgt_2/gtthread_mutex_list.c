#include<stdio.h>
#include<stdlib.h>
#include<gtthread_types.h>
#include<gtthread_test.h>

/**
 * Append new node to mutex list
 */
void appendMutex(gtthread_mutex_node **list, gtthread_mutex_t* mutex)
{
  gtthread_mutex_node *temp;
  gtthread_mutex_node *node;

	
  node = (gtthread_mutex_node *)malloc(sizeof(gtthread_mutex_node));
  node->mutex = mutex;
  node->link = NULL;

  if(*list == NULL)
  {
       *list = node;
  }
  else
  {
      temp = *list;

      while(temp->link != NULL)
      {
          temp = temp->link;
      }

      temp->link = node;
  }
}


/**
 * Delete a mutex from mutex list
 */
void deleteMutex(gtthread_mutex_node **list, long mutexID)
{
  gtthread_mutex_node *old, *temp;

  temp = *list;

  while(temp != NULL)
  {

      if(temp->mutex->mutexID = mutexID)
      {
          if(temp == *list)
          {
              *list = temp->link;
          }   
          else
          {
              old->link = temp->link;
          }

          free(temp);
          return;
       }
       else
       {
           old = temp;
           temp = temp->link;

       }
    
  }

  fprintf(stderr,"\nAttempting to remove a unlinked child thread"); 

}

/**
 * Check If Mutex Exists
 */
int checkIfMutexExists(gtthread_mutex_t mutex, gtthread_mutex_node* listOfMutexes)
{
    gtthread_mutex_node* ptr = listOfMutexes;

    while(ptr != NULL)
    {
        if(ptr->mutex->mutexID == mutex.mutexID)
        {
            return 1;
         }
    }

    return 0;

}

/**
 * Display
 */
void displayMutexList(gtthread_mutex_node* listOfMutexes)
{

  fprintf(stddebug, "\nLOG: Displaying List Contents");  
	while(listOfMutexes != NULL)
	{
		fprintf(stddebug, "\nLOG: Mutex ID: %d", listOfMutexes->mutex->mutexID);
		listOfMutexes = listOfMutexes->link;

	}

  fprintf(stddebug, "\nLOG: End of List");
}
