/*
 * test1.c
 * 	Scott Cooper
 * 	260503452
 *
 *  ECSE 427/COMP 310
 */


#include <stdio.h>

#include "MyThreads.h"

void test1(){

	semaphore_wait(0);

	printf("HERE1\n");

	semaphore_signal(0);

	mythread_exit();
}

void test2(){
	printf("HERE2\n");

	semaphore_signal(0);

	mythread_exit();
}

void test3(){
	semaphore_wait(0);
	printf("HERE3\n");
	mythread_exit();
}

int main(){
	mythread_init();
	mythread_create("TEST1", &test1, 64000);
	mythread_create("TEST2", &test2, 64000);
	mythread_create("TEST3", &test3, 64000);

	int sem = create_semaphore(0);

	runthreads();

	destroy_semaphore(sem);

	mythread_state();

	return 0;
}
