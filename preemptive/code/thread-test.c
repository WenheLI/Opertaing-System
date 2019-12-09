#include "rpi.h"
#include "rpi-thread.h"

// trivial test.   you should write a few that are better, please!
volatile int thread_count, thread_sum;
static void thread_code(void *arg) {
	int* a;
	if(rpi_cur_thread() -> tid == 0) {
		a = shmat(0, sizeof(int));
		printk("Create a value %d,\n", *a);
		*a = 1;
	} else if (rpi_cur_thread() -> tid == 1){
		a = shmget(0);
		printk("Read a value %d,\n", *a);
		*a += 1;
	} else {
		a = shmget(0);
		printk("Read a value %d,\n", *a);
	}

	thread_count ++;
	thread_sum += (unsigned)arg;
	rpi_yield();
}

// check that we can fork/yield/exit and start the threads package.
void main(void) {
	printk("running threads\n");
	int n = 3;
	thread_sum = thread_count = 0;
	for(int i = 0; i < n; i++) 
		rpi_fork(thread_code, (void*)i);
	rpi_thread_start(0);

	// no more threads: check.
	printk("count = %d, sum=%d\n", thread_count, thread_sum);
	assert(thread_count == n);
}

void notmain() {
        uart_init();
		main();
	clean_reboot();
}