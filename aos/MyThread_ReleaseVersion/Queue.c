#include "Queue.h"

/*Queue operations*/
//Add a thread to the tail of the queue
void addToQueue(queue *Q, ThreadStruct *thr)
{
    queue *newNode = (queue*) malloc(sizeof(queue));
    queue *temp = (queue*) malloc(sizeof(queue));
    newNode -> thread = thr;
    newNode -> link = NULL;
    if(Q == NULL)
    {
        Q = newNode;
    }
    else
    {
        temp = Q;
        while(temp -> link != NULL)
        {
            temp = temp -> link;
        }
        temp -> link = newNode;
    }
    readyQ = Q;         //I found this necessary in cases where Q becomes null
}

//Remove the front element from the queue and return its thread
ThreadStruct* removeFromQueue(queue *Q)
{
    ThreadStruct *result = NULL;
    if(Q != NULL)
    {
        result = Q -> thread;
        Q = Q -> link;
    }
    readyQ = Q;
    return result;
}
