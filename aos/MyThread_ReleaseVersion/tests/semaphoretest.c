/*
#include<stdio.h>
#include<stdlib.h>
#include"mythread.h"

void mainthread();
void firstchild();

int sharedint;
MySemaphore sem;

void mainthread()
{
	printf("\n mainthread: Entered mainthread..");
	MyThread fc=MyThreadCreate(firstchild,NULL);
	sem=MySemaphoreInit(1);
	printf("\n mainthread: Accessing the shared variable.waiting..");
	MySemaphoreWait(sem);
	MyThreadYield();
	sharedint=5;
	printf("\n mainthread: value of shared int is %d",sharedint);
	MySemaphoreSignal(sem);

	printf("\n mainthread: Exiting mainthread..");
	MyThreadExit();
}

void firstchild()
{
	printf("\n firstchild: Entered firstchild...");

	MySemaphoreWait(sem);
	sharedint=10;
	printf("\n firstchild: value of sharedint is %d",sharedint);
	MySemaphoreSignal(sem);

	printf("\n firstchild: Exiting firstchild...");
	MyThreadExit();
}


int main(int argc, char** argv) {

    MyThreadInit(mainthread,NULL);
    printf("\n main: Sem test: Back to main after init");
    MyThreadRun();

    printf("\n main:Back to main. Exiting now..\n");

    return (EXIT_SUCCESS);
}
*/
