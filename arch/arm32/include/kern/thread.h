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
#ifndef THREAD_H
#define THREAD_H

#include <sys/types.h>

#define THREAD_ID_0		0
#define THREAD_ABT_STACK	0xfffffffe
#define THREAD_TMP_STACK	0xffffffff

struct thread_smc_args {
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	uint32_t a3;
	uint32_t a4;	/* Thread ID when returning from RPC */
	uint32_t a5;
	uint32_t a6;	/* Optional session ID */
	uint32_t a7;	/* Hypervisor Client ID */
};

/*
 * Suspends current thread and temorarily exits to non-secure world.
 * This function returns later when non-secure world returns.
 *
 * The purpose of this function is to deliver IRQs to non-secure world
 * and to request services from non-secure world.
 */
uint32_t thread_rpc(uint32_t rv0, uint32_t rv1, uint32_t rv2, uint32_t rv3);

struct thread_abort_regs {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t ip;
	uint32_t lr;
	uint32_t spsr;
	uint32_t pad;
};
typedef void (*thread_abort_handler_t)(uint32_t abort_type,
			struct thread_abort_regs *regs);
struct thread_svc_regs {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t lr;
	uint32_t spsr;
};
typedef void (*thread_svc_handler_t)(struct thread_svc_regs *regs);
typedef void (*thread_call_handler_t)(struct thread_smc_args *args);
typedef void (*thread_fiq_handler_t)(void);
struct thread_handlers {
	/*
	 * stdcall and fastcall are called as regular functions and
	 * normal ARM Calling Convention applies. Return values are passed
	 * args->param{1-3} and forwarded into r0-r3 when returned to
	 * non-secure world.
	 *
	 * stdcall handles calls which can be preemted from non-secure
	 * world. This handler is executed with a large stack.
	 *
	 * fastcall handles fast calls which can't be preemted. This
	 * handler is executed with a limited stack. This handler must not
	 * cause any aborts or reenenable FIQs which are temporarily masked
	 * while executing this handler.
	 *
	 * TODO execute fastcalls and FIQs on different stacks allowing
	 * FIQs to be enabled during a fastcall.
	 */
	thread_call_handler_t stdcall;
	thread_call_handler_t fastcall;

	/*
	 * fiq is called as a regular function and normal ARM Calling
	 * Convention applies.
	 *
	 * This handler handles FIQs which can't be preemted. This handler
	 * is executed with a limited stack. This handler must not cause
	 * any aborts or reenenable FIQs which are temporarily masked while
	 * executing this handler.
	 */
	thread_fiq_handler_t fiq;

	/*
	 * The SVC handler is called as a normal function and should do
	 * a normal return. Note that IRQ is masked when this function
	 * is called, it's permitted for the function to unmask IRQ.
	 */
	thread_svc_handler_t svc;

	/*
	 * The abort handler is called as a normal function and should do
	 * a normal return. The abort handler is called when an undefined,
	 * prefetch abort, or data abort exception is received. In all
	 * cases the abort handler is executing in abort mode. If IRQ is
	 * unmasked in the abort handler it has to have separate abort
	 * stacks for each thread.
	 */
	thread_abort_handler_t abort;
};
void thread_init_handlers(const struct thread_handlers *handlers);

/*
 * Sets the stacks to be used by the different threads. Use THREAD_ID_0 for
 * first stack, THREAD_ID_0 + 1 for the next and so on.
 *
 * If stack_id == THREAD_ID_TMP_STACK the temporary stack used by current
 * CPU is selected.
 * If stack_id == THREAD_ID_ABT_STACK the abort stack used by current CPU
 * is selected.
 *
 * Returns true on success and false on errors.
 */
bool thread_init_stack(uint32_t stack_id, vaddr_t sp);

/*
 * Set Thread Specific Data (TSD) pointer together a function
 * to free the TSD on thread_exit.
 */
typedef void (*thread_tsd_free_t)(void *tsd);
void thread_set_tsd(void *tsd, thread_tsd_free_t free_func);

/* Returns Thread Specific Data (TSD) pointer. */
void *thread_get_tsd(void);

#endif /*THREAD_H*/
