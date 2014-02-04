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
#include <kern/thread.h>
#include <kern/thread_defs.h>
#include "thread_private.h"
#include <sm/sm_defs.h>
#include <arm32.h>
#include <kern/mutex.h>
#include <kern/misc.h>
#include <kern/arch_debug.h>
#include <kprintf.h>

#include <assert.h>

static struct thread_ctx threads[NUM_THREADS];

static struct thread_core_local thread_core_local[NUM_CPUS];

thread_call_handler_t thread_stdcall_handler_ptr;
static thread_call_handler_t thread_fastcall_handler_ptr;
static thread_fiq_handler_t thread_fiq_handler_ptr;
thread_svc_handler_t thread_svc_handler_ptr;
thread_abort_handler_t thread_abort_handler_ptr;

static struct mutex thread_global_lock = MUTEX_INITIALIZER;

static void lock_global(void)
{
	mutex_lock(&thread_global_lock);
}

static void unlock_global(void)
{
	mutex_unlock(&thread_global_lock);
}

static struct thread_core_local *get_core_local(void)
{
	struct thread_core_local *l;
	uint32_t cpu_id = get_core_pos(read_mpidr());

	assert(cpu_id < NUM_CPUS);
	l = &thread_core_local[cpu_id];
	return l;
}

static void thread_copy_args_to_ctx(struct thread_smc_args *args,
		struct thread_ctx_regs *regs)
{
	/*
	 * Copy arguments into context. This will make the
	 * arguments appear in r0-r7 when thread is resumed.
	 */
	regs->r0 = args->smc_func_id;
	regs->r1 = args->param1;
	regs->r2 = args->param2;
	regs->r3 = args->param3;
	regs->r4 = args->param4;
	regs->r5 = args->param5;
	regs->r6 = args->param6;
	regs->r7 = args->param7;
}

static void thread_alloc_and_run(struct thread_smc_args *args)
{
	size_t n;
	struct thread_core_local *l = get_core_local();
	bool found_thread = false;

	assert(l->curr_thread == -1);

	lock_global();

	for (n = 0; n < NUM_THREADS; n++) {
		if (threads[n].state == THREAD_STATE_FREE) {
			threads[n].state = THREAD_STATE_ACTIVE;
			found_thread = true;
			break;
		}
	}

	unlock_global();

	if (!found_thread) {
		args->smc_func_id = SMC_CALL_RETURN;
		args->param1 = SMC_RETURN_TRUSTED_OS_ENOTHR;
		args->param2 = 0;
		args->param3 = 0;
		args->param4 = 0;
		return;
	}

	l->curr_thread = n;

	threads[n].regs.pc = (uint32_t)thread_stdcall_entry;
	/* Stdcalls starts in SVC mode with unmasked IRQ or FIQ */
	threads[n].regs.cpsr = CPSR_MODE_SVC;
	/* Enable thumb mode if it's a thumb instruction */
	if (threads[n].regs.pc & 1)
		threads[n].regs.cpsr |= CPSR_T;
	/* Reinitialize stack pointer */
	threads[n].regs.svc_sp = threads[n].stack_va_end;
	thread_copy_args_to_ctx(args, &threads[n].regs);

	/* Save Hypervisor Client ID */
	threads[n].hyp_clnt_id = args->param7;

	thread_resume(&threads[n].regs);
}

static void thread_resume_from_rpc(struct thread_smc_args *args)
{
	size_t n = args->param4; /* thread id */
	struct thread_core_local *l = get_core_local();
	bool thread_id_ok = false;

	assert(l->curr_thread == -1);

	lock_global();

	if (n < NUM_THREADS && threads[n].state == THREAD_STATE_SUSPENDED) {
		thread_id_ok = true;
		threads[n].state = THREAD_STATE_ACTIVE;
	}

	unlock_global();

	if (!thread_id_ok || args->param7 != threads[n].hyp_clnt_id) {
		args->smc_func_id = SMC_CALL_RETURN;
		args->param1 = SMC_RETURN_TRUSTED_OS_EBADTHR;
		args->param2 = 0;
		args->param3 = 0;
		args->param4 = 0;
		return;
	}

	l->curr_thread = n;

	/*
	 * Return from RPC to request service of an IRQ must not
	 * get parameters from non-secure world.
	 */
	if (threads[n].flags & THREAD_FLAGS_COPY_ARGS_ON_RETURN) {
		thread_copy_args_to_ctx(args, &threads[n].regs);
		threads[n].flags &= ~THREAD_FLAGS_COPY_ARGS_ON_RETURN;
	}

	thread_resume(&threads[n].regs);
}

void thread_handle_smc_call(struct thread_smc_args *args)
{
	check_canaries();

	if (SMC_IS_FAST_CALL(args->smc_func_id)) {
		if (args->smc_func_id == SMC_CALL_HANDLE_FIQ) {
			thread_fiq_handler_ptr();
			/* Return to monitor with this call number */
			args->smc_func_id = SMC_CALL_RETURN_FROM_FIQ;
		} else {
			thread_fastcall_handler_ptr(args);
			/* Return to monitor with this call number */
			args->smc_func_id = SMC_CALL_RETURN;
		}
	} else {
		if (args->smc_func_id == SMC_CALL_RETURN_FROM_RPC)
			thread_resume_from_rpc(args);
		else
			thread_alloc_and_run(args);
	}
}

void *thread_get_tmp_sp(void)
{
	struct thread_core_local *l = get_core_local();

	return (void *)l->tmp_stack_va_end;
}

void thread_state_free(void)
{
	struct thread_core_local *l = get_core_local();

	assert(l->curr_thread != -1);

	lock_global();

	assert(threads[l->curr_thread].state == THREAD_STATE_ACTIVE);
	threads[l->curr_thread].state = THREAD_STATE_FREE;
	threads[l->curr_thread].flags = 0;
	l->curr_thread = -1;

	unlock_global();
}

void thread_state_suspend(uint32_t flags, uint32_t cpsr, uint32_t pc)
{
	struct thread_core_local *l = get_core_local();

	assert(l->curr_thread != -1);

	check_canaries();

	lock_global();

	assert(threads[l->curr_thread].state == THREAD_STATE_ACTIVE);
	threads[l->curr_thread].flags &= ~THREAD_FLAGS_COPY_ARGS_ON_RETURN;
	threads[l->curr_thread].flags |=
		flags & THREAD_FLAGS_COPY_ARGS_ON_RETURN;
	threads[l->curr_thread].regs.cpsr = cpsr;
	threads[l->curr_thread].regs.pc = pc;
	threads[l->curr_thread].state = THREAD_STATE_SUSPENDED;
	l->curr_thread = -1;

	unlock_global();
}


bool thread_init_stack(uint32_t thread_id, vaddr_t sp)
{
	switch (thread_id) {
	case THREAD_TMP_STACK:
		{
			struct thread_core_local *l = get_core_local();

			l->tmp_stack_va_end = sp;
			l->curr_thread = -1;
		}
		break;

	case THREAD_ABT_STACK:
		thread_set_abt_sp(sp);
		break;

	case THREAD_IRQ_STACK:
		thread_set_irq_sp(sp);
		break;

	default:
		if (thread_id >= NUM_THREADS)
			return false;
		if (threads[thread_id].state != THREAD_STATE_FREE)
			return false;

		threads[thread_id].stack_va_end = sp;
	}

	return true;
}

void thread_init_handlers(const struct thread_handlers *handlers)
{
	thread_stdcall_handler_ptr = handlers->stdcall;
	thread_fastcall_handler_ptr = handlers->fastcall;
	thread_fiq_handler_ptr = handlers->fiq;
	thread_svc_handler_ptr = handlers->svc;
	thread_abort_handler_ptr = handlers->abort;
	thread_init_vbar();
}

void thread_set_tsd(void *tsd, thread_tsd_free_t free_func)
{
	struct thread_core_local *l = get_core_local();

	assert(l->curr_thread != -1);
	assert(threads[l->curr_thread].state == THREAD_STATE_ACTIVE);
	threads[l->curr_thread].tsd = tsd;
	threads[l->curr_thread].tsd_free = free_func;
}

void *thread_get_tsd(void)
{
	struct thread_core_local *l = get_core_local();
	int ct = l->curr_thread;

	if (ct == -1 || threads[ct].state != THREAD_STATE_ACTIVE)
		return NULL;
	else
		return threads[ct].tsd;
}

struct thread_ctx_regs *thread_get_ctx_regs(void)
{
	struct thread_core_local *l = get_core_local();

	assert(l->curr_thread != -1);
	return &threads[l->curr_thread].regs;
}
