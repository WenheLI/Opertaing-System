  @empty stub routines.  use these, or make your own.

.globl rpi_get_cpsr
rpi_get_cpsr:
	mrs r0, CPSR
	bx lr

.globl rpi_cswitch
rpi_cswitch:
	sub r13, r13, #64

	str r0, [r13, #0] 
	str r1, [r13, #4]
	str r2, [r13, #8]
	str r3, [r13, #12]
	str r4, [r13, #16]
	str r5, [r13, #20]
	str r6, [r13, #24]
	str r7, [r13, #28]
	str r8, [r13, #32]
	str r9, [r13, #36]
	str r10, [r13, #40]
	str r11, [r13, #44]
	str r12, [r13, #48]
	str r14, [r13, #56]

	mrs r2, cpsr
	str r2, [r13, #60]

	str r13, [r0]    

	ldr r13, [r1]

	ldr r0, [r13, #60] 
	msr cpsr, r0

	ldr r0, [r13, #0]
	ldr r1, [r13, #4]
	ldr r2, [r13, #8]
	ldr r3, [r13, #12]
	ldr r4, [r13, #16]
	ldr r5, [r13, #20]
	ldr r6, [r13, #24]
	ldr r7, [r13, #28]
	ldr r8, [r13, #32]
	ldr r9, [r13, #36]
	ldr r10, [r13, #40]
	ldr r11, [r13, #44]
	ldr r12, [r13, #48]
	ldr r14, [r13, #56]
	add r13, r13, #64

	bx lr

.globl kernel_rpi_cswitch
kernel_rpi_cswitch:
	sub sp, sp, #64

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

	add sp, sp, #64

	mov pc, lr



@ [Make sure you can answer: why do we need to do this?]
@
@ use this to setup each thread for the first time.
@ setup the stack so that when cswitch runs it will:
@	- load address of <rpi_init_trampoline> into LR
@	- <code> into r1, 
@	- <arg> into r0
@ 
.globl rpi_init_trampoline
rpi_init_trampoline:
	@ mov r2, sp
	@ mov r3, lr
	@ b check_regs

	blx r1
	b rpi_exit
