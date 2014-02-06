#include<stdio.h>
#include<gtthread.h>

void dummy()
{
	return "hello!";
}


int main()
{

  int a = 5;	
  void* returnVal;	  
  gtthread_t* thread1;
 
  gtthread_init(50000);
  
  thread1 = (gtthread_t*)malloc(sizeof(gtthread_t));
  gtthread_create(thread1, &dummy, NULL);

  while(a < 99)
  {
    a++;
  }
  fprintf(stderr, "im here");
  
  gtthread_join(*thread1, &returnVal);
  fprintf(stderr, "\nReturn value %s", (char*)returnVal); 
  return 0;
}

