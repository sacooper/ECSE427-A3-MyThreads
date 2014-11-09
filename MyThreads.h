/*
 * MyThreads.c
 * 	Scott Cooper
 * 	260503452
 *2
 *  ECSE 427/COMP 310
 */

#ifndef ASSIGNMENT3_THREADPACKAGE_MYTHREADS_H_
#define ASSIGNMENT3_THREADPACKAGE_MYTHREADS_H_

#include <sys/queue.h>
#include <time.h>
#include <ucontext.h>

#define MAX_THREADS 64
#define MAX_SEM 64
#define QUANTUM_INIT 1000;

enum RUNNING_STATE {
	RUNNABLE, BLOCKED, EXIT
};

typedef struct thread_queue_entry {
	int id;
	TAILQ_ENTRY(thread_queue_entry) tailq;
} thread_queue_entry;

typedef struct thread_control_block {
	ucontext_t *context;
	char* thread_name;
	int id;
	enum RUNNING_STATE state;
	clock_t time;
} thread_control_block;

typedef struct sem_entry {
	int count;
	int initial;
	TAILQ_HEAD(, thread_queue_entry) *waiting_queue;
} sem_entry;

int mythread_init();
int mythread_create(char *threadname, void (*threadfunc)(), int stacksize);
void mythread_exit();
void runthreads();
void set_quantum_size(int quantum);
int create_semaphore();
void semaphore_wait(int semaphore);
void semaphore_signal(int semaphore);
void destroy_semaphore(int semaphore);
void mythread_state();

#endif /* ASSIGNMENT3_THREADPACKAGE_MYTHREADS_H_ */
