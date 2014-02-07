#include<stdio.h>
#include<gtthread.h>

#define NO_OF_PHILOSOPHERS 5

typedef enum philState
{
    DUMMY,	
    EATING,
    HUNGRY,
    THINKING
}state;


gtthread_mutex_t chopstickLock[NO_OF_PHILOSOPHERS];
gtthread_mutex_t monitor;
state philosopherState[NO_OF_PHILOSOPHERS];
int number_of_tries[5];

void checkIfCanEat(int i);
void perform(int i);
void attemptToEat(int i);
void giveUpForks(int i);

#define RAND_NUM 567
#define MOD_VAL 676
/**
 * Philosopher ID
 */
void perform(int i)
{
  int b = 0, low;
    while(1)
    {
        //TO EMULATE THINKING
        //fprintf(stderr, "\nPhilosopher %d is now thinking", i);
//        sleep(5);
        
        int a = 0;
        //TRY TO  PICK CHOPSTICKS
        //fprintf(stderr, "\nPhilosopher %d is now hungry", i);
      
	low = 1000000; 
	for(a = 0; a < NO_OF_PHILOSOPHERS; a++)
	{
		//fprintf(stderr, "%d ", number_of_tries[a]);		
		if(number_of_tries[a] < low)
			low = number_of_tries[a];

	}

	if(number_of_tries[i] - low > 1)
	{
		//fprintf(stderr, "\n%d has already eaten %d", i, number_of_tries[i]);
	}
	else
       { 	attemptToEat(i);


		//fprintf(stderr, "\nPhilosopher %d is now eating", i);

	//        sleep(3); //Assuming he takes 3 seconds to eating

		giveUpForks(i);


		number_of_tries[i] = number_of_tries[i] + 1;
	}	
        //Philosopher resumes thinking

	b++;	
     }

}

void attemptToEat(int i)
{
    //gtthread_mutex_lock(&monitor);
   
    philosopherState[i] = HUNGRY;

    checkIfCanEat(i);

    //gtthread_mutex_unlock(&monitor);

    //gtthread_mutex_lock(&chopstickLock[i]);
}

void giveUpForks(int i)
{
    int left, right;
    //gtthread_mutex_lock(&monitor);
      
    philosopherState[i] = THINKING;

    left = (i + NO_OF_PHILOSOPHERS - 1)% NO_OF_PHILOSOPHERS;
    right = (i) % NO_OF_PHILOSOPHERS;

	if(i % 2 == 0)
	{
		gtthread_mutex_unlock(&chopstickLock[right]);
		gtthread_mutex_unlock(&chopstickLock[left]);

	}
	else
	{
		gtthread_mutex_unlock(&chopstickLock[left]);
		gtthread_mutex_unlock(&chopstickLock[right]);
	}
    

    //gtthread_mutex_unlock(&monitor);  
    attemptToEat(left);
    attemptToEat(right);

    
}

void checkIfCanEat(int i)
{
    int left, right, low, a;


     right =(i + 1) % NO_OF_PHILOSOPHERS;
     left = (i + NO_OF_PHILOSOPHERS - 1)% NO_OF_PHILOSOPHERS;



    if(philosopherState[i] == HUNGRY  && philosopherState[left] != EATING && philosopherState[right] != EATING)
    {
        philosopherState[i] = EATING;
	fprintf(stderr, "\nPhil %d is now eating",i);
        
	if(i % 2 == 0)
	{
		gtthread_mutex_lock(&chopstickLock[left]);
		gtthread_mutex_lock(&chopstickLock[right]);
	}
	else
	{
		gtthread_mutex_lock(&chopstickLock[right]);
		gtthread_mutex_lock(&chopstickLock[left]);
	}

	//gtthread_mutex_unlock(&chopstickLock[i]);
    }
    else
    {
	//gtthread_yield();
    }	

    //gtthread_mutex_unlock(&chopstickLock[i]);

}


int main()
{
    gtthread_t threads[NO_OF_PHILOSOPHERS];
    int i = 0;

    gtthread_init(50);	

    for(i = 0; i < NO_OF_PHILOSOPHERS; i++)
    {
	gtthread_mutex_init(&chopstickLock[i]);
	number_of_tries[i] = 0;
    }	
    
    for(i = 0; i < NO_OF_PHILOSOPHERS; i++)
    {
        //threads[i] = (gtthread_t*)malloc(sizeof(gtthread_t));
        gtthread_create(&threads[i], &perform, i);

    }

    while(1)
    {;}	
    return 0;
}

