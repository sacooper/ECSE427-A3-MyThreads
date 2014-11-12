all: MyThreads.h MyThreads.c test1.c
	gcc -o test1 test1.c MyThreads.c -DHAVE_PTHREAD_RW_LOCK=1
