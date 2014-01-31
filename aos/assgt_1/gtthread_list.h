#ifndef GTTHREAD_LIST_H
#define GTTHREAD_LIST_H

int checkIfThreadExists(gtthread_t thread, gtthread_node *listOfThreads);
void appendThread(gtthread_node **list, gtthread_t* thread);
void deleteThread(gtthread_node **list, long threadID);

#endif
