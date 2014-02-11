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
#include <sm/sm_defs.h>

#include <kern/mmu.h>
#include <kern/kern.h>
#include <kern/resmem.h>
#include <kern/arch_debug.h>

#include <arm32.h>
#include <kern/thread.h>
#include <kern/panic.h>

#include <tee/entry.h>

#include <assert.h>

#ifdef WITH_STACK_CANARIES
#define STACK_CANARY_SIZE	(4 * sizeof(uint32_t))
#define START_CANARY_VALUE	0xdededede
#define END_CANARY_VALUE	0xabababab
#define GET_START_CANARY(name, stack_num) name[stack_num][0]
#define GET_END_CANARY(name, stack_num) \
	name[stack_num][sizeof(name[stack_num]) / sizeof(uint32_t) - 1]
#else
#define STACK_CANARY_SIZE	0
#endif

#define DECLARE_STACK(name, num_stacks, stack_size) \
	static uint32_t name[num_stacks][(stack_size + STACK_CANARY_SIZE) / \
					 sizeof(uint32_t)] \
		__attribute__((section(".bss.prebss.stack"), \
			       aligned(STACK_ALIGMENT)))

#define GET_STACK(stack) \
	((vaddr_t)(stack) + sizeof(stack) - STACK_CANARY_SIZE / 2)


DECLARE_STACK(stack_tmp,	NUM_CPUS,	STACK_TMP_SIZE);
DECLARE_STACK(stack_abt,	NUM_CPUS,	STACK_ABT_SIZE);
DECLARE_STACK(stack_sm,		NUM_CPUS,	SM_STACK_SIZE);
DECLARE_STACK(stack_thread,	NUM_THREADS,	STACK_THREAD_SIZE);

const vaddr_t stack_tmp_top[NUM_CPUS] = {
	GET_STACK(stack_tmp[0]),
#if NUM_CPUS > 1
	GET_STACK(stack_tmp[1]),
#endif
#if NUM_CPUS > 2
	GET_STACK(stack_tmp[2]),
#endif
#if NUM_CPUS > 3
	GET_STACK(stack_tmp[3]),
#endif
#if NUM_CPUS > 4
#error "Top of tmp stacks aren't defined for more than 4 CPUS"
#endif
};

static bool inited;

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



static void init_canaries(void)
{
	size_t n;
#define INIT_CANARY(name)						\
	for (n = 0; n < ARRAY_SIZE(name); n++) {			\
		uint32_t *start_canary = &GET_START_CANARY(name, n);	\
		uint32_t *end_canary = &GET_END_CANARY(name, n);	\
									\
		*start_canary = START_CANARY_VALUE;			\
		*end_canary = END_CANARY_VALUE;				\
		kprintf("#Stack canaries for %s[%zu] with top at %p\n", \
			#name, n, (void *)(end_canary - 1));		\
		kprintf("#watch inited && *%p\n", (void *)start_canary);\
		kprintf("watch inited && *%p\n", (void *)end_canary);	\
	}

	INIT_CANARY(stack_tmp);
	INIT_CANARY(stack_abt);
	INIT_CANARY(stack_sm);
	INIT_CANARY(stack_thread);
}

void check_canaries(void)
{
#ifdef WITH_STACK_CANARIES
	size_t n;

#define ASSERT_STACK_CANARIES(name)					\
	for (n = 0; n < ARRAY_SIZE(name); n++) {			\
		assert(GET_START_CANARY(name, n) == START_CANARY_VALUE);\
		assert(GET_END_CANARY(name, n) == END_CANARY_VALUE);	\
	} while (0)

	ASSERT_STACK_CANARIES(stack_tmp);
	ASSERT_STACK_CANARIES(stack_abt);
	ASSERT_STACK_CANARIES(stack_sm);
	ASSERT_STACK_CANARIES(stack_thread);
#endif /*WITH_STACK_CANARIES*/
}

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

	/* Initialize canries around the stacks */
	init_canaries();

	if (!thread_init_stack(THREAD_TMP_STACK, GET_STACK(stack_tmp[0])))
		panic();
	if (!thread_init_stack(THREAD_ABT_STACK, GET_STACK(stack_abt[0])))
		panic();
	for (n = 0; n < NUM_THREADS; n++) {
		if (!thread_init_stack(n, GET_STACK(stack_thread[n])))
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
	sm_init(GET_STACK(stack_sm[0]));
	nsec_ctx = sm_get_nsec_ctx();
	nsec_ctx->mon_lr = nsec_entry;
	nsec_ctx->mon_spsr = CPSR_MODE_SVC | CPSR_I;

	/* Initialize GIC */
	gic_init(mmu_map_device(GIC_BASE + GICC_OFFSET, 0x1000),
		mmu_map_device(GIC_BASE + GICD_OFFSET, 0x1000));
	gic_it_add(IT_UART1);
	gic_it_set_cpu_mask(IT_UART1, 0x1);
	gic_it_set_prio(IT_UART1, 0xff);
	gic_it_enable(IT_UART1);

	inited = true;
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
	uint32_t iar;

	kprintf("%s\n", __func__);

	iar = gic_read_iar();

	while (uart_have_rx_data(UART1_BASE))
		kprintf("got 0x%x\n", uart_getchar(UART1_BASE));

	gic_write_eoir(iar);

	kprintf("return from %s\n", __func__);
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
