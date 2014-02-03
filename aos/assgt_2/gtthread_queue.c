#include<stdio.h>
#include<stdlib.h>
#include<gtthread_types.h>
#include<gtthread_queue.h>
#include<gtthread_test.h>

/**
 * Initialize the Queue
 */
void initQueue(gtthread_queue** q)
{
  gtthread_queue *queue = *q;

  queue->front = queue->rear = NULL;
	queue->count = 0;
}

/**
 * Add Thread to Queue
 */
void addThreadToQueue(gtthread_queue** q, gtthread_t* thread)
{
    gtthread_node *temp;
    gtthread_queue  *queue;

    queue = *q;
    temp = (gtthread_node*)malloc(sizeof(gtthread_node)); 
    temp->thread = thread;
    temp->link = NULL;

  	queue->count = queue->count + 1;
	
    if(queue->front == NULL)
    {
        queue->rear = queue->front = temp;
        return;
    }

    queue->rear->link = temp;
    queue->rear = queue->rear->link;
}

/**
 * Remove Thread from front of Queue
 */
gtthread_t* removeThreadFromQueue(gtthread_queue** q)
{
    gtthread_node *temp;
    gtthread_t* thread;    
    gtthread_queue *queue;

    queue = *q;

    if(queue->front == NULL)
    {
      printf("\nQueue is empty.");
      return NULL;
    }

    thread = queue->front->thread;
    temp = queue->front;

    queue->front = queue->front->link;
    
    free(temp);

    queue->count = queue->count - 1;

    return thread;
}

/**
 * Display Queue
 */
void displayQueue(gtthread_queue* queue)
{
	gtthread_node* ptr;
	ptr = queue->front;

//	fprintf(stddebug, "\nLOG: Displaying Queue");

  fprintf(stddebug, "\nLOG: ");

	while(ptr != NULL)
	{
		fprintf(stddebug, "(%ld, %d) ", ptr->thread->threadID, ptr->thread->state); 
		ptr = ptr->link;
	}

  fprintf(stddebug, "\nLOG: End of Queue");

}


