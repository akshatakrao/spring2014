#ifndef GTTHREAD_QUEUE_H
#define GTTHREAD_QUEUE_H

#include<gtthread_types.h>

typedef struct queue
{
  gtthread_node *front;
  gtthread_node *rear;
}gtthread_queue;

void initQueue (gtthread_queue* queue);
void addThreadToQueue(gtthread_queue* queue, gtthread_t* thread);
gtthread_t* removeThreadFromQueue(gtthread_queue* queue);

#endif
