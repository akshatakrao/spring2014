#ifndef GTTHREAD_MUTEX_LIST_H
#define GTTHREAD_MUTEX_LIST_H

void appendMutex(gtthread_mutex_node **list, gtthread_mutex_t* mutex);
void deleteMutex(gtthread_mutex_node **list, long mutexID);
int checkIfMutexExists(gtthread_mutex_t mutex, gtthread_mutex_node* listOfMutexes);
void displayMutexList(gtthread_mutex_node* listOfMutexes);

#endif
