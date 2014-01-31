#include <stdio.h>
#include<gtthread_types.h>
#include<gtthread_list.h>


void gtthread_init(long period)
{
    if(period <= 0)
    {
        fprintf(stderr,"\nQuantum period must be positive");
        exit(INVALID_QUANTUM_PERIOD);
    }
    quantum_period = period;  
  
}



