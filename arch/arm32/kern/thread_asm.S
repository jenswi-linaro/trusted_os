/*
 * Copyright (c) 2014, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <asm.S>
#include <arm32.h>
#include <sm/sm_defs.h>
#include <kern/thread_defs.h>

FUNC thread_set_abt_sp , :
	mrs	r1, cpsr
	cps	#CPSR_MODE_ABT
	mov	sp, r0
	msr	cpsr, r1
	bx	lr
END_FUNC thread_set_abt_sp

FUNC thread_recv_smc_call , :
	push	{r0-r7}
	mov	r0, sp
	bl	thread_handle_smc_call
	/*
	 * Normally thread_handle_smc_call() should return via
	 * thread_exit(), thread_rpc(), but if thread_handle_smc_call()
	 * hasn't switched stack (fast call, FIQ, error detected) it will
	 * do a normal "C" return.
	 */
	pop	{r0-r7}
	smc	#0
	b	thread_recv_smc_call	/* Next entry to secure world is here */
END_FUNC thread_recv_smc_call

/* void thread_resume(struct thread_ctx_regs *regs) */
FUNC thread_resume , :
	add	r12, r0, #(12 * 4)	/* Do the general purpose regs later */

	cps	#CPSR_MODE_SYS
	ldm	r12!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_IRQ
	ldm	r12!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_SVC
	ldm	r12!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_ABT
	ldm	r12!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_UND
	ldm	r12!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_SVC
	ldm	r12, {r1, r2}
	push	{r1, r2}

	ldm	r0, {r0-r11}


	/* Restore CPSR and jump to the intstruction to resume at */
	rfefd	sp!
END_FUNC thread_resume

/*
 * Disables IRQ and FIQ and saves state of thread, returns original
 * CPSR.
 */
LOCAL_FUNC thread_save_state , :
	push	{r12, lr}
	/*
	 * Uses stack for temporary storage, while storing needed
	 * context in the thread context struct.
	 */

	mrs	r12, cpsr

	cpsid	if			/* Disable IRQ and FIQ */

	push	{r4-r7}
	push	{r0-r3}

	mov	r5, r12			/* Save CPSR in a preserved register */
	mrs	r6, cpsr		/* Save current CPSR */

	bl	thread_get_ctx_regs

	pop	{r1-r4}			/* r0-r3 pushed above */
	stm	r0!, {r1-r4}
	pop	{r1-r4}			/* r4-r7 pushed above */
	stm	r0!, {r1-r2}
	stm	r0!, {r8-r11}

        cps     #CPSR_MODE_SYS
        mrs     r1, spsr
        stm     r0!, {r1, sp, lr}

        cps     #CPSR_MODE_IRQ
        mrs     r1, spsr
        stm     r0!, {r1, sp, lr}

        cps     #CPSR_MODE_SVC
        mrs     r1, spsr
        stm     r0!, {r1, sp, lr}

        cps     #CPSR_MODE_ABT
        mrs     r1, spsr
        stm     r0!, {r1, sp, lr}

        cps     #CPSR_MODE_UND
        mrs     r1, spsr
        stm     r0!, {r1, sp, lr}

	msr	cpsr, r6		/* Restore mode */

	mov	r0, r5			/* Return original CPSR */
	pop	{r12, pc}
END_FUNC thread_save_state

FUNC thread_stdcall_entry , :
	/* Pass r0-r7 in a struct thread_smc_args */
	push	{r0-r7}
	mov	r0, sp
	ldr	lr, =thread_stdcall_handler_ptr
	ldr	lr, [lr]
	blx	lr
	/*
	 * Load the returned r0-r3 into preserved registers and skip
	 * the "returned" r4-r7 since they will not be returned.
	 */
	pop	{r4-r7}
	add	sp, #(4 * 4)

	/* Disable interrupts before switching to temporary stack */
	cpsid	if
	bl	thread_get_tmp_sp
	mov	sp, r0

	bl	thread_state_free

	mov	r0, #SMC_CALL_RETURN
	mov	r1, r4
	mov	r2, r5
	mov	r3, r6
	mov	r4, r7
	smc	#0
	b	thread_recv_smc_call	/* Next entry to secure world is here */
END_FUNC thread_stdcall_entry


/*
 * uint32_t thread_rpc(uint32_t rv0, uint32_t rv1, uint32_t rv2, uint32_t rv3)
 */
FUNC thread_rpc , :
	push	{r12, lr}
	push	{r0-r3}

	bl	thread_save_state
	mov	r4, r0			/* Save original CPSR */

	/*
 	 * Switch to temporary stack and SVC mode. Save CPSR to resume into.
	 */
	bl	thread_get_tmp_sp
	pop	{r4-r7}			/* Originally pushed r0-r3 */
	cps	#CPSR_MODE_SVC		/* Change to SVC mode */
	mov	sp, r0			/* Switch to tmp stack */
	push	{r4-r7}			/* Save original r0-r3 again */

	mov	r0, #THREAD_FLAGS_COPY_ARGS_ON_RETURN
	mov	r1, r4			/* CPSR to restore */
	ldr	r2, .thread_rpc_return
	bl	thread_state_suspend
	pop	{r0-r3}
	smc	#0
	b	. 			/* "Can't happen" */

.thread_rpc_return:
	/*
	 * Jumps here from thread_resume above when RPC has returned. The
	 * IRQ and FIQ bits are restored to what they where when this
	 * function was originally entered.
	 */
	pop	{r12, lr}
	bx	lr
END_FUNC thread_rpc

LOCAL_FUNC thread_irq_handler , :
	sub	lr, lr, #4
	push	{ip, lr}

	bl	thread_save_state
	mov	r4, r0			/* Save original CPSR */

	/*
 	 * Switch to temporary stack and SVC mode. Save CPSR to resume into.
	 */
	bl	thread_get_tmp_sp
	cps	#CPSR_MODE_SVC		/* Change to SVC mode */
	mov	sp, r0			/* Switch to tmp stack */
	
	mov	r0, #0
	mov	r1, r4			/* CPSR to restore */
	ldr	r2, .thread_irq_return
	blx	thread_state_suspend

	mov	r0, #SMC_RETURN_TRUSTED_OS_RPC
	mov	r1, #SMC_RPC_REQUEST_IRQ
	mov	r2, #0
	mov	r3, #0
	smc	#0
	b	.			/* "Can't happen" */

.thread_irq_return:
	/*
	 * Jumps here from thread_resume above when RPC has returned. The
	 * IRQ and FIQ bits are still set, but will be restored from SPSR
	 * when this function returns.
	 */
	pop	{ip, lr}
	movs	pc, lr
END_FUNC thread_irq_handler

FUNC thread_init_vbar , :
	/* Set vector (VBAR) */
	ldr	r0, =thread_vect_table
	mcr	p15, 0, r1, c12, c0, 0
	bx	lr
END_FUNC thread_init_vbar

LOCAL_FUNC thread_abort_handler , :
thread_abort_handler:
thread_und_handler:
	/*
	 * Switch to abort mode to use that stack instead.
	 */
	cps	#CPSR_MODE_ABT
	sub	lr, lr, #4
	push	{r0-r3, ip, lr}
	cps	#CPSR_MODE_UND
	mrs	r0, spsr
	cps	#CPSR_MODE_ABT
	push	{r0, r1}
	mov	r0, #THREAD_ABORT_UNDEF
	b	.thread_abort_generic

thread_dabort_handler:
	sub	lr, lr, #8
	push	{r0-r3, ip, lr}
	mrs	r0, spsr
	push	{r0, r1}
	mov	r0, #THREAD_ABORT_PREFETCH
	b	.thread_abort_generic

thread_pabort_handler:
	sub	lr, lr, #4
	push	{r0-r3, ip, lr}
	mrs	r0, spsr
	push	{r0, r1}
	mov	r0, #THREAD_ABORT_DATA
	b	.thread_abort_generic

.thread_abort_generic:
	mov	r1, sp
	ldr	lr, =thread_abort_handler_ptr;
	ldr	lr, [lr]
	blx	lr
	pop	{r0, r1}
	msr	spsr, r0
	pop	{r0-r3, ip, lr}
	movs	pc, lr
END_FUNC thread_abort_handler

LOCAL_FUNC thread_svc_handler , :
	push	{r0-r5, lr}
	mrs	r0, spsr
	push	{r0}
	mov	r0, sp
	ldr	lr, =thread_svc_handler_ptr;
	ldr	lr, [lr]
	blx	lr
	pop	{r0}
	msr	spsr, r0
	pop	{r0-r5, lr}
	movs	pc, lr
END_FUNC thread_svc_handler

        .align	5
LOCAL_FUNC thread_vect_table , :
	b	.			/* Reset			*/
	b	thread_und_handler	/* Undefined instruction	*/
	b	thread_svc_handler	/* System call			*/
	b	thread_pabort_handler	/* Prefetch abort		*/
	b	thread_dabort_handler	/* Data abort			*/
	b	.			/* Reserved			*/
	b	thread_irq_handler	/* IRQ				*/
	b	.			/* FIQ				*/
END_FUNC thread_vect_table
