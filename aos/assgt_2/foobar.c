#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<signal.h>
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c - Auto-generated by Anjuta's Makefile project wizard
 * 
 */
struct itimerval timeslice;
struct sigaction handler;

void printMain();

int main()
{
    timeslice.it_value.tv_sec = 0;
    timeslice.it_value.tv_usec = 500000;
    timeslice.it_interval = timeslice.it_value;
//    setitimer(ITIMER_VIRTUAL, &timeslice, NULL);

    memset(&handler, 0 , sizeof(handler));
    handler.sa_handler=printMain;
    sigaction(SIGVTALRM, &handler, NULL) ;
    setitimer(ITIMER_VIRTUAL, &timeslice, NULL);

    while(1);
       
}

void printMain()
{
    printf("\nIm Main");

}

