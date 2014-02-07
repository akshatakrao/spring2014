#include<stdio.h>
#include<gtthread.h>

#define NO_OF_PHILOSOPHERS 5


gtthread_mutex_t chopsticks[NO_OF_PHILOSOPHERS];
gtthread_t threads[NO_OF_PHILOSOPHERS];

void pickChopsticks(int i)
{
    int left, right;

    left = (i+1)%NO_OF_PHILOSOPHERS;
    right = (i+NO_OF_PHILOSOPHERS - 1)%NO_OF_PHILOSOPHERS;

    if(i%2 == 1)
    {
      gtthread_mutex_unlock(&chopsticks[left]);
      gtthread_mutex_lock(&chopsticks[right]);
    }
    else
    {
        gtthread_mutex_lock(&chopsticks[right]);
        gtthread_mutex_lock(&chopsticks[left]);
    }

    fprintf(stderr, "\nPhilosopher %d is eating", i);
}


void putdownChopsticks(int i)
{

        int left, right;

    left = (i+1)%NO_OF_PHILOSOPHERS;
    right = (i+NO_OF_PHILOSOPHERS - 1)%NO_OF_PHILOSOPHERS;

    if(i%2 == 1)
    {
      gtthread_mutex_lock(&chopsticks[right]);
      gtthread_mutex_lock(&chopsticks[left]);
    }
    else
    {
        gtthread_mutex_lock(&chopsticks[left]);
        gtthread_mutex_lock(&chopsticks[right]);

    }
}

void tryToEat(int i)
{
    fprintf(stderr, "\nPhilosopher %d tries to eat");
}

void initialize()
{
    void* a;
    int i = 0;
    
    gtthread_init(10000);

    for(i = 0; i < NO_OF_PHILOSOPHERS; i++)
    {
        gtthread_mutex_init(&chopsticks[i]);
    }

    for(i = 0; i <NO_OF_PHILOSOPHERS; i++)
    {
        a = &i;
        gtthread_create(&threads[i], tryToEat, a);
    }



}

