#include "rpi.h"
#include "rpi-thread.h"
#include "timer-interrupt.h"

#define E rpi_thread

#include "queue.h"

static Queue run_queue, freed_queue;
static unsigned nthreads = 0;

static rpi_thread *cur_thread;  // current running thread.
static rpi_thread *scheduler_thread; // first scheduler thread.
static rpi_shm* shms[10];
int shms_count = 0;

int shmdest(int shmid) {
	if (shmid < 0 || shmid >= 10) return -1;
	shms[shmid] -> init = -7264;
	return 1;
	
}

void* shmat(int shmid, size_t size) {
	if (shmid < 0 || shmid >= 10) return NULL;
	rpi_shm* s = shms[shmid];
	if (s->init == -7264) {
		s->init = 1;
		void* temp = kmalloc(size);
		// printk("id: %d\n", shmid);
		s->target = temp;
	} else {
		printk("this id has been inited\n");
		return NULL;
	}
	return s->target;
}

void* shmget(int shmid) {
	if (shmid < 0 || shmid >= 10) return -1;
	rpi_shm* s = shms[shmid];
	// printk("id::: %d", shmid);
	if (s->init) {
		return s->target;
	} else {
		printk("this id has not been inited\n");
		return NULL;
	}
}


// call kmalloc if no more blocks, otherwise keep a cache of freed threads.
static rpi_thread *mk_thread(void) {
	rpi_thread *t;

	// if (nthreads == 0) {
	// 	run_queue = *Q_init();
	// 	freed_queue =* Q_init();
	// }

	if(!(t = Q_pop(&freed_queue))) t = kmalloc(sizeof(rpi_thread));
	t -> tid = nthreads;
	t -> quant = 3;
	t -> sp = (uint32_t*) t -> stack + 1204 * 8;
	nthreads += 1;
	return t;
}

void int_handler(unsigned pc) {
	unsigned pending = RPI_GetIRQController()->IRQ_basic_pending;

	if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
		return;
    RPI_GetArmTimer()->IRQClear = 1;
	
	cur_thread -> quant -= 1;
	printk("Task:\n");
	// printk("%d\n", cur_thread->quant);
	if (cur_thread -> quant == 0) {
		// printk("Get zero\n");
		kernel_rpi_yield();
	}

}

// create a new thread.
rpi_thread *rpi_fork(void (*code)(void *arg), void *arg) {
	rpi_thread *t = mk_thread();
	enum { 
		// register offsets are in terms of byte offsets!
		LR_offset = 14,  
		CPSR_offset = 15,
		R0_offset = 0, 
		R1_offset = 1, 
	};

	// write this so that it calls code,arg.
	void rpi_init_trampoline(void);

	t -> sp = ((uint32_t*)t -> sp - 1 - 16);
	t -> sp[LR_offset] = rpi_init_trampoline;
	t -> sp[R0_offset] = arg;
	t -> sp[R1_offset] = code;
	t -> sp[CPSR_offset] = rpi_get_cpsr();

  	printk("Add thread to run queue \n");
	Q_append(&run_queue, t);
	return t;
}

// exit current thread.
void rpi_exit(int exitcode) {
	printk("Exiting \n");
	Q_append(&freed_queue, cur_thread);
	rpi_thread* old = cur_thread;
	cur_thread = Q_pop(&run_queue);
	if(cur_thread) {
		rpi_cswitch(&old->sp, &cur_thread->sp);	
	} else {
		rpi_cswitch(&old->sp, &scheduler_thread->sp);
	}
}

// yield the current thread.
void rpi_yield() {
  // reset quant 
  cur_thread -> quant = 3;
  Q_append(&run_queue, cur_thread);
  rpi_thread* old = cur_thread;
  cur_thread = Q_pop(&run_queue);
  printk("Switching from %d to %d\n", old->tid, cur_thread->tid);
  if(cur_thread) {
    rpi_cswitch(&old->sp, &cur_thread->sp);
  } else {
    cur_thread = old;
  }
}


void kernel_rpi_yield() {
  cur_thread -> quant = 3;
  Q_append(&run_queue, cur_thread);
  rpi_thread* old = cur_thread;
  cur_thread = Q_pop(&run_queue);
  printk("Switching from %d to %d\n", old->tid, cur_thread->tid);
  if(cur_thread) {
    kernel_rpi_cswitch(&old->sp, &cur_thread->sp);
  } else {
    cur_thread = old;
  }
}

// starts the thread system: nothing runs before.
// 	- <preemptive_p> = 1 implies pre-emptive multi-tasking.
void rpi_thread_start(int preemptive_p) {
  if (preemptive_p) {
	  printk("premptive\n");
	  install_int_handlers();
	  timer_interrupt_init(0x5);
	  system_enable_interrupts();
  }
  cur_thread = Q_pop(&run_queue);
  if(cur_thread) {
    scheduler_thread = mk_thread();


    rpi_cswitch(&scheduler_thread->sp, &cur_thread->sp);
  } else {
    printk("No threads!\n");
    clean_reboot();
  }


	printk("THREAD: done with all threads, returning\n");
	if(preemptive_p) system_disable_interrupts();
}

rpi_thread *rpi_cur_thread(void) {
	return cur_thread;
}