compile:
	clear
	gcc -o gtthread gtthread.c gtthread_test.c gtthread_list.c gtthread_queue.c -I .

clean:
		rm *.o

execute:
		./gtthread || cat debug.log
