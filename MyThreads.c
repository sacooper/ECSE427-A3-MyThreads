/*
 * MyThreads.c
 * 	Scott Cooper
 * 	260503452
 *
 *  ECSE 427/COMP 310
 */

#include "MyThreads.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Strings representing the different thread states
char* state_string[]={"RUNNABLE", "BLOCKED", "EXIT"};

// Queue of running processes
TAILQ_HEAD(, thread_queue_entry) running_queue;

// Table containing all currently running threads
thread_control_block threads[MAX_THREADS];

// Table containing all current in use semaphores
sem_entry sems[MAX_SEM];

// Time since last swap
clock_t last_swap;

// sigaction struct for SIGALRM
struct sigaction sa;

int thread_count,	// Total number of threads
	sem_count,		// Total number of threads
	quantum,		// Current quantum size
	current_thread;	// ID of currently executing thread

// Timer to control SIGALRM
struct itimerval tval;

// Cleanup function executed at_exit to free memory
void cleanup(){
	int i;
	for (i = 0; i < thread_count; i++){
		free(threads[i].context->uc_stack.ss_sp);
		free(threads[i].context);
	}
	thread_queue_entry *item;
    while ((item = TAILQ_FIRST(&running_queue))) {
		TAILQ_REMOVE(&running_queue, item, tailq);
		free(item);
    }
	for (i = 0; i < sem_count; i++){
		free(sems[i].waiting_queue);
	}
}

// Create a new thread. Returns 1 if true
int mythread_init() {
	TAILQ_INIT(&running_queue);	// Initialize running queue
	thread_count = 1;
	sem_count = 0;

	quantum = QUANTUM_INIT;
	current_thread = 0;

	// Create thread_control_block for the main thread
	thread_control_block main_thread;

	main_thread.context = malloc(sizeof(ucontext_t));

	if (main_thread.context == NULL)// malloc failed
		return -1;

	getcontext(main_thread.context);

	main_thread.context->uc_stack.ss_sp = malloc(1024*64);

	if (main_thread.context->uc_stack.ss_sp == NULL)	// malloc failed
		return -1;
	main_thread.context->uc_stack.ss_size = sizeof(main_thread.context->uc_stack);
	main_thread.state = RUNNABLE;
	main_thread.time = clock();
	main_thread.thread_name = "main_thread";
	main_thread.id = 0;
	threads[0] = main_thread;

	atexit(cleanup);

	return 0;
}

// Create a new thread named 'threadname' to execute function 'threadfunc' with a given
// stacksize

int mythread_create(char *threadname, void (*threadfunc)(), int stacksize) {
	// Create a new thread_control_block
	thread_control_block *new = &(threads[thread_count]);

	new->context = malloc(sizeof(ucontext_t));

	if (new->context == NULL)
		return -1;

	// Create a new context for this thread
	getcontext(new->context);
	new->context->uc_stack.ss_sp = malloc(stacksize);
	if (new->context->uc_stack.ss_sp == NULL)
		return -1;
	new->context->uc_stack.ss_size = stacksize;
	new->context->uc_link = threads[0].context;
	new->thread_name = strdup(threadname);
	new->time = 0;
	new->state = RUNNABLE;
	new->id = thread_count++;
	makecontext(new->context, threadfunc, 0);

	// Add this thread to the runnable queue
	thread_queue_entry *this = malloc(sizeof(thread_queue_entry));

	if (this == NULL)
		return -1;

	this->id = new->id;
	TAILQ_INSERT_TAIL(&running_queue, this, tailq);

	return 0;
}

// Perform the switch between current thread and next in runqueue
void thread_switch(){
	if (!TAILQ_EMPTY(&running_queue)){	// only swap if there's something to swap
		thread_control_block *current, *next;
		thread_queue_entry *next_t;

		current = &(threads[current_thread]);
		next_t = TAILQ_FIRST(&running_queue);
		next = &(threads[next_t->id]);
		current_thread = next_t->id;

		TAILQ_REMOVE(&running_queue, next_t, tailq);
		free(next_t);
		clock_t now;
		now = clock();
		current->time += (now - last_swap);
		last_swap = clock();

		// Save the current thread if it is still runnable (not blocked/exited)
		if (current->state == RUNNABLE && current->id != 0){
			thread_queue_entry *this = malloc(sizeof(thread_queue_entry));
			this->id = current->id;
			TAILQ_INSERT_TAIL(&running_queue, this, tailq);}

		swapcontext(current->context, next->context);

	}

}

// Exit a thread, setting the state to EXIT
void mythread_exit() {
	sigprocmask(SIG_BLOCK, &(sa.sa_mask), NULL);
	thread_control_block *current = &(threads[current_thread]);
	current->state = EXIT;
	clock_t now = clock();
	current->time += (now - last_swap);
	last_swap = clock();
	sigprocmask(SIG_UNBLOCK, &(sa.sa_mask), NULL);
}

// Run the set of all runnable threads
void runthreads() {
	/* Install timer_handler as the signal handler for SIGVTALRM. */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = thread_switch;
	sigaddset(&(sa.sa_mask), SIGALRM);
	sigaction(SIGALRM, &sa, NULL);
	tval.it_interval.tv_sec = 0;
	tval.it_interval.tv_usec = quantum;
	tval.it_value.tv_sec = 0;
	tval.it_value.tv_usec = quantum;
	last_swap = clock();

	if (!TAILQ_EMPTY(&running_queue)){	// only swap if there's something to swap
			thread_control_block *main_thread = &(threads[0]);
			thread_queue_entry *next_t = TAILQ_FIRST(&running_queue);
			thread_control_block *next = &(threads[next_t->id]);
			current_thread = next_t->id;
			TAILQ_REMOVE(&running_queue, next_t, tailq);
			free(next_t);
			clock_t now;
			now = clock();
			main_thread->time += (now - last_swap);
			last_swap = clock();

			setitimer(ITIMER_REAL, &tval, 0);
			int running = 1, i = 0;
			swapcontext(main_thread->context, next->context);
			while(running){
				running = 0;
				for (i = 1; i <=thread_count; i++){

					if (threads[i].state != EXIT)
						running = 1;
				}
			}
	}
}

// change the quantum size and reset the timer
void set_quantum_size(int new_quantum) {
	quantum = new_quantum;
	tval.it_interval.tv_sec = 0;
	tval.it_interval.tv_usec = quantum;
	tval.it_value.tv_sec = 0;
	tval.it_value.tv_usec = quantum;
	setitimer(ITIMER_REAL, &tval, 0);
}

// Create a new semaphore
int create_semaphore(int value) {
	sem_entry *new = &(sems[sem_count]);
	new->count = value;
	new->initial = value;
	new->waiting_queue = malloc(sizeof(TAILQ_HEAD(, thread_queue_entry)));

	if (new->waiting_queue == NULL)	// malloc failed
		return -1;
	TAILQ_INIT(new->waiting_queue);

	return sem_count++;
}

// Decrement the value of a given semaphore, and block thread if count is negative
void semaphore_wait(int semaphore) {
	sigprocmask(SIG_BLOCK, &(sa.sa_mask), NULL);
	sem_entry *sem = &(sems[semaphore]);
	(sem->count)--;
	if (sem->count < 0){
		thread_control_block *current = &(threads[current_thread]);
		thread_queue_entry *this = malloc(sizeof(thread_queue_entry));
		this->id = current->id;
		current->state = BLOCKED;
		TAILQ_INSERT_TAIL(sem->waiting_queue, this, tailq);
		sigprocmask(SIG_UNBLOCK, &(sa.sa_mask), NULL);
		thread_switch();
	}
	sigprocmask(SIG_UNBLOCK, &(sa.sa_mask), NULL);
}

// Signal a semaphore, increasing the value
void semaphore_signal(int semaphore) {
	sigprocmask(SIG_BLOCK, &(sa.sa_mask), NULL);
	sem_entry *this = &(sems[semaphore]);
	(this->count)++;
	if (this->count >= 0){	// If we have a value >= 0, unblock first process in wait queue
		if (!TAILQ_EMPTY(this->waiting_queue)){
			thread_queue_entry *first = TAILQ_FIRST(this->waiting_queue);
			threads[first->id].state = RUNNABLE;
			TAILQ_REMOVE(this->waiting_queue, first, tailq);
			thread_queue_entry *to_insert = malloc(sizeof(thread_queue_entry));
			to_insert->id = first->id;
			free(first);
			TAILQ_INSERT_TAIL(&running_queue, to_insert, tailq);
		}
	}
	sigprocmask(SIG_UNBLOCK, &(sa.sa_mask), NULL);
}

// Destroy a semaphore with a given id
void destroy_semaphore(int semaphore) {
	sem_entry this = sems[semaphore];
	if (!TAILQ_EMPTY(this.waiting_queue)){	// Make sure no threads are waiting on this semaphore
		fprintf(stderr, "Cannot destroy semaphore %d. Threads are still waiting on it\n", semaphore);
		exit(1);
	} else {
		if (this.count != this.initial)		// Warn if count is what it was when semapohre was created
			printf("WARNING: Current value of semaphore %d (%d) does not equal initial value (%d)\n",
					semaphore, this.count, this.initial);
	}
}

// Print out the current state of the program in table form
void mythread_state() {
	int i;
	printf("%3s: %15s %10s %8s\n", "ID", "Thread Name", "State", "Time");
	printf("----------------------------------------\n");
	while(threads[i].context != NULL){
		printf("%3d: %15s %10s %8d\n", i, threads[i].thread_name, state_string[threads[i].state], threads[i].time);
		i++;
	}
}

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

	create_semaphore(0);

	runthreads();

	mythread_state();

	return 0;
}