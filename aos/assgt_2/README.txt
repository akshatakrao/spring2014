Akshata Rao
903013116


This is an implementation of the threading library using context calls provided by ucontext.

The Platform used:
------------------
Ubuntu 12.04.4 LTS

Implementation of the Preemptive Scheduler
-------------------------------------------
	The scheduler is implemented as a round robin preemptive scheduler. A queue is maintained containing the threads that have been created. This includes the main thread as well.

	We maintain a thread node(structure) that keeps links to the thread state, the blocking threads and its owned mutexes. The ready queue is but a queue of these thread nodes.

	The main thread is the first to be placed in the ready queue. Whenever the scheduler is called, the scheduler checks for
         1. Is the thread at the head of the queue in the ready state? If yes, execute it by swapping the context to the thread's context.
	 2. If the thread is awaiting for a mutex,yield to the next contending thread.
	 3. If the thread has other threads that it has joined too and are yet blocking, yield the scheduler for the next thread in queue.
         4. If a thread has blocking threads whose state is TERMINATED/CANCELLED, change the thread state to READY and continue its execution


The quantum is implemented using a timer and a signal handler. The timer/signal handler listens to the SIGVTALRM signal and starts the scheduling mechanism. After the expiry of the timer, the scheduler is called again. Upon every call, a context switch is made between the threads at the front most of the queue. If the thread has not completed even after its quantum has expired, it is placed at the back of the queue.

Compilation of Program
-------------------------


Deadlocks in Dining Philosopher
--------------------------------

The deadlock free solution for the dining philosopher is
1. Pick up both chopsticks, and not one at a time.
2. Ensure that the right and left philosophers are not eating at the time you wish to eat.
3. Every chopstick is treated as a mutex resource.
4. A monitor is used to lock the critical section of the philosopher code that picks up the chopsticks and drops them.


Thoughts on the Project
------------------------
1. A very interesting project to understand the nuances of scheduling
2. The time quantum sometimes gives unpredictable behaviour while trying out interleaving. This is frustrating sometimes.
3. The setivtimer is also not entirely reliable at times.
4. Context switching in C, context calls were really good learning.

	

	
	

