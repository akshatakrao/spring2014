/*
#include <stdio.h>
#include <stdlib.h>
#include "mythread.h"


void firstchild();
void mainthread();

void mainthread() {
	MyThread child1;
    printf("\n mainthread: Inside the mainthread. Trying to create firstchild");
    child1 = MyThreadCreate(firstchild,NULL);
    MyThreadYield();
    printf("\n mainthread: Created the firstchild successfully. Its id is %d", child1);
    printf("\n");
    MyThreadYield();
    printf("\n mainthread: Hello world! Have fun threading! mainthread exiting.. \n");
    MyThreadExit();
}

void firstchild()
{
    printf("\n firstchild: Entered the first child! Performed my first ever context switch!!!!!");
    MyThreadYield();
    printf("\n firstchild: Exiting the first child now");
    MyThreadYield();
    MyThreadExit();
}



int main(int argc, char** argv) {

    MyThreadInit(mainthread,NULL);
    printf("\n main: Back to main after init");
    MyThreadRun();

    printf("\n main:Back to main. Exiting now..\n");

    return (EXIT_SUCCESS);
}
*/
