#include<stdio.h>
#include<gtthread.h>


#define NO_OF_PHILOSOPHERS 5

typedef enum philState
{
    DUMMY,
    EATING,
    THINKING
}state;


state philStates[NO_OF_PHILOSOPHERS];
gtthread_mutex_t forks[NO_OF_PHILOSOPHERS];
gtthread_mutex_t monitor;
gtthread_t philosophers[NO_OF_PHILOSOPHERS];

void tryEating(int i)
{
    int left,right;

    gtthread_mutex_lock(&monitor);

    fprintf(stderr, "\nPhiPhilosopher  %d", i);

    left = (i+1)%NO_OF_PHILOSOPHERS;
    right = (i+NO_OF_PHILOSOPHERS-1)%NO_OF_PHILOSOPHERS;

    if(philStates[left] != EATING && philStates[right] != EATING)
    {
        philStates[i] = EATING;
        gtthread_mutex_lock(&forks[left]);
        gtthread_mutex_lock(&forks[right]);
    }
    gtthread_mutex_unlock(&monitor);

}


int main()
{
  int i = 0;

  for(i = 0 ; i < NO_OF_PHILOSOPHERS; i++)
  {
      gtthread_mutex_init(&forks[i]);
  }

  gtthread_mutex_init(&monitor);
  gtthread_init(10000);

  for(i = 0; i < NO_OF_PHILOSOPHERS; i++)
  {
      gtthread_create(&philosophers[i], tryEating, i);
  }

  while(1)
  {;}

  return 0;
}


