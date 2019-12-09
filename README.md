# Opertaing-System# Docmuents on RaspberryPi

## Index
1. [How to boot](#boot)
2. [Bootloader](#Bootloader)
3. [Shell](#Shell)
4. [Scheduler](#Scheduler)
5. [FileSystem](#FileSystem)

**Note:** The base code is from CS140e, a course for operating system. It provides abstraction mainly for `print`, `malloc`, `timer`, and `mem-barrier`.

## boot
### How does raspberry pi boot a program?
Raspberry PI has a set of firmware that helps you do the initial booting process with a `config txt` file and an `elf` file for GPU booting.  After the GPU got booted, it will seek and read an `image` file with a fixed name to memory at 0x80000 and that makes it possible to run our programs with Raspberry PI. 

![](https://i.imgur.com/AFKXdvd.png)

***Note:*** Different version of Raspberry PI will seek for different name of `image` file to boot. Below is a table for your reference:

| Raspberry PI Version |Image name | 
| -------- | -------- | 
| Raspberry Pi 3| kernel8.img| 
| Raspberry Pi 2| kernel7.img| 
| Raspberry Pi | kernel.img| 

Also, the firmware for GPU booting can be found [here](https://github.com/raspberrypi/firmware/tree/master/boot).

### How to build an image for Raspberry PI

To build an image for Raspberry PI we need the following essential parts:
- Linker Script:
    - Linker Script is to set the base address for an image and allocate sections like `text` and `global`.
    Below is a short script that gives the definition of `text` address, `data` address and `base` address.
    ```
    SECTIONS
    {

        .text 0x8000 :  { KEEP(*(.text.boot))  *(.text*)}
        .data : { *(.data*) } 
        .rodata : { *(.rodata*) }
        .bss : {
            __bss_start__ = .;
            *(.bss*)
        *(COMMON)
            . = ALIGN(8);
            __bss_end__ = .;
        . = ALIGN(8);
            __heap_start__ = .;
        }
    }
    ```
    - C code
    This is the major codebase for Operating System.
    - Asm code
    We need a snippet assembly code that can be understood directly by CPU to run our entry functions.
    ```asm
    .section ".text.boot"

    .globl _start
    _start:
        // dwelch starts here: mov sp, #0x8000
        mov sp, #0x8000000
        mov fp, #0  
        @ bl notmain
        bl _cstart // entry function
        bl rpi_reboot // reboot function 
    ```
    
## Bootloader
Recall how Raspberry PI boots a program. If we want to load another program, we need to overwrite the SD card and re-plugin the Raspberry PI.
With the `bootloader`, we can write our image and load it directly into Raspberry Pi’s memory without replacing the SD card. 

### UART
In detail, we need firstly to implement `UART(Universal Asynchronous Receiver/Transmitter)`. Since we are accessing hardware via addresses, we need to keep using the `voliate` keyword to ensure no optimization for memory access.
The UART requires three steps:
- init the hardware
```c
void uart_init() {
	dev_barrier();
	
	gpio_set_function(GPIO_TX, GPIO_FUNC_ALT5);
	gpio_set_function(GPIO_RX, GPIO_FUNC_ALT5);
	
	dev_barrier();

	//enable aux
	volatile unsigned *aux_enb  = (volatile unsigned *)(AUX_BASE + 0x04);
	int x = get32(aux_enb);
	x |= 1;
	put32(aux_enb, x);
	
	dev_barrier();

	//disable transmit receive
	volatile unsigned *aux_mu_cntl = (volatile unsigned *)(AUX_BASE + 0x60);
	put32(aux_mu_cntl, 0);

	//set the baud rate
	volatile unsigned *aux_baud = (volatile unsigned *)(AUX_BASE + 0x68);
	put32(aux_baud, 270);

	//look at lcr errata, write a 3
	volatile unsigned *aux_mu_lcr = (volatile unsigned *)(AUX_BASE + 0x4c);
	put32(aux_mu_lcr, 3);

	//clear transmit and receive FIFO
	volatile unsigned *aux_mu_iir = (volatile unsigned *)(AUX_BASE + 0x48);
	put32(aux_mu_iir, 6);


	//enable transmit receive 
	put32(aux_mu_cntl, 3);

	dev_barrier();
}
```
- get function
```c
int uart_getc(void) {
	while(1) {
		if (!(get32(aux_mu_lsr) & 1)) break;
	}
	return get32(aux_mu_io) & 0xFF;
}
```
- put function
```c
void uart_putc(unsigned c) {
	while(1) {
		if ((get32(aux_mu_lsr) & (1 << 5))) break;
	}
	put32(aux_mu_io + 8, c & 0xFF);
}

```

### Communication
With `UART` we can transmit information between our computer and Raspberry PI. However, we still need to ensure no packages are missing during this process. Thus, we introduced `checksum` to help us with validation.

### Write to memory
In the previous section, we know that ``0x8000`` is the address for booting external programs. Thus, we can directly write every byte from `UART` to the base address(``0x8000``) and branch the pointer to his address. 
Also, we need to reboot Raspberry Pi if the current program is over. This is to guarantee that our next writing behavior.

## Shell
The shell is like a medium that connects the front-end -- the computer and the back-end -- Raspberry PI. What we need to do is to take the input and use `UART` to transform it to Raspberry PI. Then Raspberry PI will give corresponding response via `UART`.

Below is a simple shell program:
```c
while((n = readline(buf, sizeof buf))) {
		
		if(strncmp(buf, "echo", 4) == 0) {
			printk("%s\n", buf + 4);
		} else if (strncmp(buf, "reboot", 6) == 0) {
			printk("PI REBOOT!!!\n");
			delay_ms(100);
			rpi_reboot();
		} else if (strncmp(buf, "run", 3) == 0) {
			printk("going to run binary\n");
			BRANCHTO(load_code());
			printk("%s", cmd_done);
		} else {
			printk("PI: > \n");
		}
	}
```

## Scheduler


The core idea of opening a thread is to load the code into the executable memory stack and adjust the `LR register` pointing to the start of the stack. This is quite like the concept of Return Oriented Programming which we need to know where to jump to make the next program executed. 

FYI, below is a table for Registers and their functionalities:

| Register | Functionality |
| -------- | -------- |
|  R0 - R3 |  Arguments |
|  R4 - R11 | Variables |
| R12 |The initial procedure call|
| R13 | Stack Pointer |
| R14 | Link Register (next return address) |
| R15 | Program Counter |

Another import register is the Current Program Status Register (CPSR), where it stores the essential data/states for a process. When we want to do a **fork** command, we need to copy the values in the register and fetch them when we need to exit from the new process.

The basic structure for a process consists of **tid, stack pointer, and stack**. The stack is the memory space for storing the codes and args of the thread. And the stack pointer points to the bottom of the stack memory space. Then it is about how to manage the memory space in the stack to let the functions get corresponding arguments. 

### Design of Thread
```c
typpdef struct thread {
	uin32_t * sp;
	uint32_t tid;
	uint32_t stack[1024*8];
    uint32_t quant;
} thread;
```

In this design, every **thread** has separate stack space for program and unique thread-id to track the thread. The **sp** is for tracking stack pointer for the thread's own stack. Also, the **quant** is for tracking how many ticks a thread is running for preemption.

### Context Switching
This is the core part of multi-programming. **Context Switching** is to switch one thread into another. The key thing here is to save all values in the old thread and fetch the values in the new thread to registers.
Below is assembly code for context switching:
```asm
kernel_rpi_cswitch:
	sub sp, sp, #64

	// restore old register
        str r0, [sp]
        str r1, [sp, #4]
        str r2, [sp, #8]
	str r3, [sp, #12]
	str r4, [sp, #16]
	str r5, [sp, #20]
	str r6, [sp, #24]
	str r7, [sp, #28]
	str r8, [sp, #32]
	str r9, [sp, #36]
	str r10, [sp, #40]
	str r11, [sp, #44]
	str r12, [sp, #48]
	str r14, [sp, #60]
	str r15, [sp, #56]
	mrs r2, cpsr
	str r2, [sp, #52]

	str sp, [r0]

	ldr sp, [r1]

	// put new memory to register
	ldr r0, [sp]
	ldr r1, [sp, #52]
	msr cpsr, r1
	ldr r1, [sp, #4]
	ldr r2, [sp, #8]
	ldr r3, [sp, #12]
	ldr r4, [sp, #16]
	ldr r5, [sp, #20]
	ldr r6, [sp, #24]
	ldr r7, [sp, #28]
	ldr r8, [sp, #32]
	ldr r9, [sp, #36]
	ldr r10, [sp, #40]
	ldr r11, [sp, #44]
	ldr r12, [sp, #48]
	ldr r14, [sp, #60]

	// restore stack pointer
	add sp, sp, #64

	// return to return address
	mov pc, lr
```

### Queue
```c
typedef struct queue {
	thread* threads[50] // max number
	Int curr_num // current number of threads;
} queue;
```
The queue data structure should have methods like pop, push, peek, and tail so that we can use such data structure to implement most scheduler algorithms. 


### fork, yield, and exit
The **fork**, **yield**, and **exit** are three major functions for thread operation.
- fork:
    The fork is to init a memory space and set register values for a thread. Besides, it **hooks** a thread-function into the memory stack. 
- exit:
    The exit works in a way that will **pop** a thread from running thread queue.
- yield:
    The yield will call **context_switch** to mount the current thread.

### Preemption
With the previous structure and the provided `timer` and `interrupt` library, we can implement the preemption by simply counting the `quant` and calling `context switch` while `quant` is zero. Or we can set an interrupt handler to manage the `quant` globally.

Below are the diagram showing the two potential solutions.
![](https://i.imgur.com/wYSCMCU.png)
![](https://i.imgur.com/N1o0XzP.png)

## FileSystem
This is still an exploration as it still remains many works to be done.
There are mainly three steps for the File System:

- SD driver:
    There is nothing like a `stdlib` in the Raspberry Pi context, which means no `write` or `read` functions are provided. That means we need to implement these functions on ourselves and in this case we need to implement a basic SD card driver.
- Hardware(Disk) structure
    ![](https://i.imgur.com/y9HP1f2.png)
    Above is a demostration of Disk Layout from cs140e official website. We need to read the MBR block.
- I-node
    This is a data structure of the file and directory. It shows how many blocks each file occupied and how to retrive each file. Also, it contains information about time, permisssion, and owner.
