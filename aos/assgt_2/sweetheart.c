#include<stdio.h>
#include<gtthread.h>

#define NO_OF_PHILOSOPHERS 5

/**
 * Philosopher 
 */
typedef struct Philosopher_Struct
{
  gtthread_mutex_t eatingMutex; //Monitor
  gtthread_mutex_t fork; //Fork Lock
  gtthread_t thread;//Thread
}philosopher;

//Philosopher Array
philosopher phils[NO_OF_PHILOSOPHERS];

//Array to keep track of the number of times each Philosopher has eaten
int number_of_tries[NO_OF_PHILOSOPHERS];

//Lock to protect number_of_tries shared datastructure
gtthread_mutex_t triesLock;


void tryToEat(int i);

/**
 * Function: Philosopher tries to eat
 * Arguments: i = philosopherID
 */
void tryToEat(int i)
{
    int left,right;

    left = (i+1)%NO_OF_PHILOSOPHERS;
    right = (i+NO_OF_PHILOSOPHERS - 1)%NO_OF_PHILOSOPHERS;
   
    gtthread_mutex_lock(&(phils[i].eatingMutex));

   fprintf(stderr, "\nPhilosopher %d is hungry", i);

    //Check if number of tries 
    int low = 100000;
    int a = 0;

    gtthread_mutex_lock(&triesLock);
    for(a = 0 ; a < NO_OF_PHILOSOPHERS; a++)
    {
       if(number_of_tries[a] < low)
          low = number_of_tries[a]; 
    }

    if(number_of_tries[i] > low)
    {
        fprintf(stderr, "\nPhilosopher %d sees other starving philosophers and yields chance this time", i);
        gtthread_mutex_unlock(&triesLock);
        gtthread_mutex_unlock(&(phils[i].eatingMutex));
        tryToEat(i);
        return;
    }

    gtthread_mutex_unlock(&triesLock);

    if(gtthread_mutex_trylock(&(phils[left].fork)) && gtthread_mutex_trylock(&(phils[right].fork)))
    {
     
        fprintf(stderr, "\nPhilosopher %d sees both forks available", i); 
        gtthread_mutex_lock(&(phils[left].fork));
        gtthread_mutex_lock(&(phils[right].fork));

        number_of_tries[i] = number_of_tries[i] + 1;
        fprintf(stderr, "\nPhilosopher %d ate!", i);

        gtthread_mutex_unlock(&(phils[right].fork));
        gtthread_mutex_unlock(&(phils[left].fork));

        fprintf(stderr, "\n Philosopher %d released both forks", i);
    }

    gtthread_mutex_unlock(&phils[i].eatingMutex);
    tryToEat(i);

}

int main3();
/**
 * Main Function
 */
int main3()
{
    int i = 0;

    //Initialize forks and monitor locks
    for(i = 0; i < NO_OF_PHILOSOPHERS; i++)
    {
          gtthread_mutex_init(&phils[i].eatingMutex);
          gtthread_mutex_init(&phils[i].fork);
    }

    //Initialize shared resource <eating tracker> variable
    gtthread_mutex_init(&triesLock);

    //Initialize thread quantum
    gtthread_init(10000);

    //Create all threads
    for(i = 0; i < NO_OF_PHILOSOPHERS; i++)
    {
        gtthread_create(&(phils[i].thread), tryToEat, (void*)i);
    }

    
  while(1)
    {
        ;
    }

  return 0;
}
