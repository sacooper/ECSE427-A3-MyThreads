/*
 * test1.c
 * 	Scott Cooper
 * 	260503452
 *
 *  ECSE 427/COMP 310
 */


#include <stdio.h>

#include "MyThreads.h"

int sem;

void a(){

	printf("a() waiting on sem: %d\n", sem);
	semaphore_wait(sem);

	printf("a()\n");

	printf("a() signaling sem: %d\n", sem);
	semaphore_signal(sem);

	printf("a() exiting\n");
	mythread_exit();
}

void b(){
	printf("b()\n");

	printf("b() signaling sem: %d\n", sem);
	semaphore_signal(sem);

	printf("b() exiting\n");
	mythread_exit();
}

void c(){
	printf("c() waiting on sem: %d\n", sem);
	semaphore_wait(sem);

	printf("c()\n");

	printf("c() exiting\n");
	mythread_exit();
}

int main(){
	mythread_init();
	mythread_create("A", &a, 64000);
	mythread_create("B", &b, 64000);
	mythread_create("C", &c, 64000);

	sem = create_semaphore(0);

	runthreads();

	destroy_semaphore(sem);

	mythread_state();

	return 0;
}
