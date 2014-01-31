/*
 * File:   ThreadSpecifics.c
 * Author: aswin
 *
 * Created on August 24, 2011, 12:32 AM
 */

#include <stdio.h>
#include "ThreadSpecifics.h"


/*Operations on ThreadList*/

//Search for a particular thread using its tid and return it
ThreadStruct* searchThreadList(ThreadList *list, int thrid)
{
    ThreadStruct *result = (ThreadStruct*) malloc(sizeof(ThreadStruct));
    ThreadList *temp = (ThreadList*) malloc(sizeof(ThreadList));
    result=NULL;
    temp = list;
    if(list==NULL)
    	return NULL;
    while(temp != NULL)
    {
        if(temp -> thread -> tid == thrid)
        {
            result = temp -> thread;
            break;
        }
        else
        {
            temp = temp -> next;
        }
    }

    return result;
}

//Remove a particular thread
void removeFromThreadList(ThreadList *list, ThreadStruct *thr)
{
    int result = -1;
    ThreadList *temp = (ThreadList*) malloc(sizeof(ThreadList));
    temp = list;
    if(list == NULL) //list is empty
    {
        return;
    }
    if(temp -> thread -> tid == thr -> tid) //The first element of the thread is the one to remove
    {
    	if(temp->next != NULL)
    		temp = temp->next;
    	else
    		temp = NULL;
    }
    else        //Search through the list and remove it
    {
        while(temp -> next != NULL)
        {
            ThreadList *nextToTemp = (ThreadList*) malloc(sizeof(ThreadList));
            nextToTemp = temp -> next;
            if(nextToTemp -> thread -> tid == thr -> tid)
            {
                temp -> next = nextToTemp -> next;
                break;
            }
            temp = temp -> next;
        }
    }

}

//Add a thread at the end
void addToThreadList(ThreadList *list, ThreadStruct *thr)
{
    ThreadList *newNode = (ThreadList*) malloc(sizeof(ThreadList));
    newNode -> thread = thr;
    newNode -> next = NULL;
    if(list == NULL)
    {
        list = newNode;
        BlockedThreads = list;//Again this is because of pointer complexities.
                             //AllThreads have been taken care of..
    }
    else
    {
        newNode->next = list;
        list = newNode;
    }
}


//Get the currently running thread
ThreadStruct* getRunningThread()
{
    ThreadStruct *result = NULL;
    ThreadStruct *tempThread;
    ThreadList *tempList = AllThreads;
    while(tempList != NULL)
    {
        tempThread = tempList->thread;
        if(tempThread->state == 1)
        {
            result = tempThread;
            break;
        }
        tempList = tempList->next;
    }

    return result;
}
