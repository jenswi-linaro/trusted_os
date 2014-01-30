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
#include <stdint.h>
#include <string.h>

#include <plat.h>

#include <drivers/gic.h>
#include <drivers/uart.h>
#include <kprintf.h>
#include <sm/sm.h>

#include <kern/mmu.h>
#include <kern/resmem.h>

#include <arm32.h>
#include <kern/thread.h>
#include <kern/panic.h>

#include <tee/entry.h>

uint8_t stack_tmp[NUM_CPUS][STACK_TMP_SIZE]
	__attribute__((section(".bss.prebss.stack"), aligned(STACK_ALIGMENT)));
uint8_t stack_abt[NUM_CPUS][STACK_ABT_SIZE]
	__attribute__((section(".bss.prebss.stack"), aligned(STACK_ALIGMENT)));
uint8_t stack_irq[NUM_CPUS][STACK_IRQ_SIZE]
	__attribute__((section(".bss.prebss.stack"), aligned(STACK_ALIGMENT)));
uint8_t stack_thread[NUM_THREADS][STACK_THREAD_SIZE]
	__attribute__((section(".bss.prebss.stack"), aligned(STACK_ALIGMENT)));

uint32_t mmu_l1_table[MMU_L1_NUM_ENTRIES]
	__attribute__((section(".bss.prebss.mmu"), aligned(MMU_L1_ALIGNMENT)));

extern uint32_t __text_start;
extern uint32_t __rodata_end;
extern uint32_t __data_start;
extern uint32_t __bss_start;
extern uint32_t __bss_end;
extern uint32_t _end;
extern uint32_t _end_of_ram;

static void main_stdcall(struct thread_smc_args *args);
static void main_fastcall(struct thread_smc_args *args);
static void main_fiq(void);
static void main_svc(struct thread_svc_regs *regs);
static void main_abort(uint32_t abort_type,
	struct thread_abort_regs *regs);

static const struct thread_handlers handlers = {
	.stdcall = main_stdcall,
	.fastcall = main_fastcall,
	.fiq = main_fiq,
	.svc = main_svc,
	.abort = main_abort
};

void main_init(uint32_t nsec_entry); /* called from assembly only */
void main_init(uint32_t nsec_entry)
{
	uintptr_t code_start = (uintptr_t)&__text_start;
	uintptr_t code_end = (uintptr_t)&__rodata_end;
	uintptr_t data_start = (uintptr_t)&__data_start;
	uintptr_t bss_start = (uintptr_t)&__bss_start;
	uintptr_t bss_end = (uintptr_t)&__bss_end;
	uintptr_t begin_resmem = (uintptr_t)&_end;
	uintptr_t end_resmem = (uintptr_t)&_end_of_ram;
	struct sm_nsec_ctx *nsec_ctx;
	size_t n;

	resmem_init(begin_resmem, end_resmem);

	/* Initialize user with physical address */
	uart_init(UART1_BASE);

	kprintf_init((kvprintf_putc)uart_putc,
		(kprintf_flush_output)uart_flush_tx_fifo, (void *)UART1_BASE);

	kprintf("Trusted OS initializing\n");

	mmu_init(mmu_l1_table, code_start, code_end, data_start, end_resmem);

	/* Reinitialize with virtual address now that MMU is enabled */
	kprintf_init((kvprintf_putc)uart_putc,
		(kprintf_flush_output)uart_flush_tx_fifo,
		(void *)mmu_map_device(UART1_BASE, 0x1000));

	/*
	 * Map normal world DDR, TODO add an interface to let normal world
	 * supply this.
	 */
	mmu_map_rwmem(DDR0_BASE, DDR0_SIZE, true /*ns*/);

	/*
	 * Zero BSS area. Note that globals that would normally would go
	 * into BSS which are used before this has to be put into
	 * .bss.prebss.* to avoid getting overwritten.
	 */
	memset((void *)bss_start, 0, bss_end - bss_start);

	gic_init(mmu_map_device(GIC_BASE + GICC_OFFSET, 0x1000),
		mmu_map_device(GIC_BASE + GICD_OFFSET, 0x1000));

	if (!thread_init_stack(THREAD_TMP_STACK, (vaddr_t)stack_tmp[0],
			STACK_TMP_SIZE))
		panic();
	if (!thread_init_stack(THREAD_ABT_STACK, (vaddr_t)stack_abt[0],
			STACK_ABT_SIZE))
		panic();
	if (!thread_init_stack(THREAD_IRQ_STACK, (vaddr_t)stack_irq[0],
			STACK_IRQ_SIZE))
		panic();
	for (n = 0; n < NUM_THREADS; n++) {
		if (!thread_init_stack(0, (vaddr_t)stack_thread[n],
				STACK_THREAD_SIZE))
			panic();
	}
	/*
	 * Mask IRQ and FIQ before switch to the thread vector as the
	 * thread handler requires IRQ and FIQ to be masked while executing
	 * with the temporary stack.
	 */
	write_cpsr(read_cpsr() | CPSR_F | CPSR_I);
	thread_init_handlers(&handlers);

	/* Initialize secure monitor */
	sm_init();
	nsec_ctx = sm_get_nsec_ctx();
	nsec_ctx->mon_lr = nsec_entry;
	nsec_ctx->mon_spsr = CPSR_MODE_SVC | CPSR_F | CPSR_I;

	kprintf("Switching to normal world boot\n");
}

static void main_stdcall(struct thread_smc_args *args)
{
	kprintf("%s\n", __func__);
	tee_entry(args);
}

static void main_fastcall(struct thread_smc_args *args)
{
	kprintf("%s\n", __func__);
	tee_entry(args);
}

static void main_fiq(void)
{
	kprintf("%s\n", __func__);
}

static void main_svc(struct thread_svc_regs *regs)
{
	kprintf("%s\n", __func__);
}

static void main_abort(uint32_t abort_type,
	struct thread_abort_regs *regs)
{
	kprintf("%s 0x%x\n", __func__, abort_type);
	panic();
}
