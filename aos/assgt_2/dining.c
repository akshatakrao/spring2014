#include<stdio.h>
#include <gtthread.h>

#define NO_OF_PHILOSOPHERS 5

typedef enum state
{
    THINKING,
    HUNGRY,
    EATING
}philState;

int getLeft(int philosopherID)
{
    return (philosopherID + NO_OF_PHILOSOPHERS-1)%NO_OF_PHILOSOPHERS;
}

int getRight(int philosopherID)
{
    return (philosopherID + 1) % NO_OF_PHILOSOPHERS;
}

int mutex = 1;
int forks[NO_OF_PHILOSOPHERS];

void philosopher(int i)
{
    while(TRUE)
    {
        think();
    }
}
int main()
{

}
