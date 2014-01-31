#include<gtthread.h>
#include<gtthread_types.h>
#include<gtthread_list.h>
#include<gtthread_list.h>

/**
 * Append new node to thread list
 */
void append(gtthread_node **list, gtthread_t* thread)
{
  struct gtthread_node* temp, *node;

  node = malloc(sizeof gtthread_node);
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
*

/**
 * Delete a thread from thread list
 */
void delete(gtthread_node **list, long threadID)
{
  gtthread_node *old, *temp;

  temp = *list;

  while(temp != NULL)
  {

      if(temp->threadID = threadID)
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


