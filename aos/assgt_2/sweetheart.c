#include<stdio.h>
#include<gtthread.h>

#define NO_OF_PHILOSOPHERS 5

typedef struct Philosopher_Struct
{
  gtthread_mutex_t eatingMutex;
  gtthread_mutex_t fork;
  gtthread_t thread;
}philosopher;

philosopher phils[NO_OF_PHILOSOPHERS];
int number_of_tries[NO_OF_PHILOSOPHERS];

gtthread_mutex_t triesLock;

void attack(int i)
{
    int left,right;

    left = (i+1)%NO_OF_PHILOSOPHERS;
    right = (i+NO_OF_PHILOSOPHERS - 1)%NO_OF_PHILOSOPHERS;
   
 /*   int loop = 0;
    while(loop < (678*(i+1)%3000))
           loop++; */
    gtthread_mutex_lock(&(phils[i].eatingMutex));

   //fprintf(stderr, "\nPhilosopher %d is trying to eat", i);

    //Check if number of tries 
    int low = 100000;
    int a = 0;

    //if(gtthread_mutex_trylock(&triesLock) == 
    gtthread_mutex_lock(&triesLock);
    for(a = 0 ; a < NO_OF_PHILOSOPHERS; a++)
    {
       if(number_of_tries[a] < low)
          low = number_of_tries[a]; 
    }
    gtthread_mutex_unlock(&triesLock);  

    if(number_of_tries[i] > low)
    {
     //   fprintf(stderr, "\nLet other threads try");
        gtthread_mutex_unlock(&triesLock);
        gtthread_mutex_unlock(&(phils[i].eatingMutex));
       // fprintf(stderr, "\nReached here");
        attack(i);
        return;
    }

    gtthread_mutex_unlock(&triesLock);

    if(gtthread_mutex_trylock(&(phils[left].fork)) && gtthread_mutex_trylock(&(phils[right].fork)))
    {
      
        gtthread_mutex_lock(&(phils[left].fork));
        gtthread_mutex_lock(&(phils[right].fork));

        number_of_tries[i] = number_of_tries[i] + 1;
        fprintf(stderr, "\nPhilosopher %d ate!", i);

//        int input;
//        scanf("%d", &input);

        gtthread_mutex_unlock(&(phils[right].fork));
        gtthread_mutex_unlock(&(phils[left].fork));

       // fprintf(stderr, "\n Philosopher %d released locks %d %d", i, right, left);
    }
    else
    {
        //fprintf(stderr, "\nPhilosopher %d couldnt get locks", i);
       // gtthread_mutex_unlock(&phils[i].eatingMutex);
        //attack(i);
    }

    gtthread_mutex_unlock(&phils[i].eatingMutex);
    attack(i);

}

int main()
{
    int i = 0;

    for(i = 0; i < NO_OF_PHILOSOPHERS; i++)
    {
          gtthread_mutex_init(&phils[i].eatingMutex);
          gtthread_mutex_init(&phils[i].fork);
    }

    gtthread_mutex_init(&triesLock);
    gtthread_init(10000);

    for(i = 0; i < NO_OF_PHILOSOPHERS; i++)
    {
        gtthread_create(&(phils[i].thread), attack, (void*)i);
    }

    
  while(1)
    {
        ;
    }

  return 0;
}
