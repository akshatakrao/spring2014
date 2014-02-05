#include<stdio.h>
#include<gtthread.h>

void dummy()
{
	while(1)
	{
		printf("Thr1 hi");
	}
}


int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t* thread1;
 
  gtthread_init(500);
  
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &dummy, a);

 
gtthread_cancel(*thread1);	 
 
 
  //gtthread_exit(NULL);	  

  return 0;
}

