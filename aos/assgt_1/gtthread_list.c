//#include<gtthread_types.h>
#include<stdio.h>
#include<stdlib.h>
#include<gtthread_types.h>

/**
 * Append new node to thread list
 */
void appendThread(gtthread_node **list, gtthread_t* thread)
{
  gtthread_node *temp;
  gtthread_node *node;

  node = (gtthread_node *)malloc(sizeof(struct thread_node));
  node->thread = thread;
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
 * Delete a thread from thread list
 */
void deleteThread(gtthread_node **list, long threadID)
{
  gtthread_node *old, *temp;

  temp = *list;

  while(temp != NULL)
  {

      if(temp->thread->threadID = threadID)
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
 * Check If Thread Exists
 */
int checkIfThreadExists(gtthread_t thread, gtthread_node* listOfThreads)
{
    gtthread_node* ptr = listOfThreads;

    while(ptr != NULL)
    {
        if(ptr->thread->threadID == thread.threadID)
        {
            return 1;
         }
    }

    return 0;

}

