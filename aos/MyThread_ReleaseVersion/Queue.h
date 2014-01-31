/*
 * File:   Queue.h
 * Author: aswin
 *
 */

#include<stdlib.h>
#include "ThreadSpecifics.h"


typedef struct queue
{
    struct ThreadStruct* thread;
    struct queue* link;
}queue;

queue *readyQ;          //The ready queue

/*Queue operations*/
//Add a thread to the tail of the queue
void addToQueue(queue* Q, ThreadStruct* thr);

//Remove the front element from the queue and return its thread
ThreadStruct* removeFromQueue(queue *Q);

