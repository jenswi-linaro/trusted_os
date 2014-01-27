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
#include <arm32_macros.S>
#include <sm/sm_defs.h>

LOCAL_FUNC sm_save_modes_regs , :
	/* User mode registers has to be saved from system mode */
	cps	#CPSR_MODE_SYS
	mrs	r1, spsr
	stm	r0!, {r1, sp, lr}

	cps	#CPSR_MODE_IRQ
	mrs	r1, spsr
	stm	r0!, {r1, sp, lr}

	cps	#CPSR_MODE_SVC
	mrs	r1, spsr
	stm	r0!, {r1, sp, lr}

	cps	#CPSR_MODE_ABT
	mrs	r1, spsr
	stm	r0!, {r1, sp, lr}

	cps	#CPSR_MODE_UND
	mrs	r1, spsr
	stm	r0!, {r1, sp, lr}

	cps	#CPSR_MODE_MON
	bx	lr
END_FUNC sm_save_modes_regs

/* Restores the mode specific registers, must preserve r12 */
LOCAL_FUNC sm_restore_modes_regs , :
	/* User mode registers has to be saved from system mode */
	cps	#CPSR_MODE_SYS
	ldm	r0!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_IRQ
	ldm	r0!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_SVC
	ldm	r0!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_ABT
	ldm	r0!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_UND
	ldm	r0!, {r1, sp, lr}
	msr	spsr, r1

	cps	#CPSR_MODE_MON
	bx	lr
END_FUNC sm_restore_modes_regs

LOCAL_FUNC sm_smc_entry , :
	srsdb	sp!, #CPSR_MODE_MON
	push	{r0-r3}
/* Positions relative to stack pointer */
#define SMC_ENTRY_R0R3_OFFS	0
#define SMC_ENTRY_SRS_OFFS	(4 * 4 + SMC_ENTRY_R0R3_OFFS)

	/* Clear the exclusive monitor */
	clrex

	/* Find out if we're doing an secure or non-secure entry */
	mrc	p15, 0, r1, c1,c1, 0
	tst	r1, #SCR_NS
	bne	.smc_nsec_entry

	ldr	r1, =SMC_CALL_RETURN
	cmp	r0, r1
	beq	.smc_ret_to_nsec	/* Normal return to non-secure */

	ldr	r1, =SMC_CALL_RETURN_FROM_FIQ
	cmp	r0, r1
	beq	.smc_ret_from_fiq	/* Return from FIQ */

	ldr	r0, =SMC_CALL_UNKNOWN
	b	.smc_exit		/* Unknown secure call, return error */

.smc_ret_from_fiq:
	/*
	 * It's a return from FIQ
	 *
	 * Restore the context saved when receiving the FIQ
	 */
	bl	sm_get_pre_fiq_ctx
	ldm	r0!, {r1}
	mcr	p15, 0, r1, c1,c1, 0	/* Restore SCR */
	ldm	r0!, {r4-r7}		/* Load saved r0-r3 */
	add	r1, sp, #SMC_ENTRY_R0R3_OFFS /* Where pop below will read */
	stm	r1, {r4-r7}		/* Write r0-r3 in stack */
	ldm	r0!, {r4-r12}
	bl	sm_restore_modes_regs
	ldm	r0, {r2-r3} 		/* Load SPSR and LR from context */
	add	r1, sp, #SMC_ENTRY_SRS_OFFS /* Where rfefd will read */
	stm	r1, {r2-r3}		/* Store SPSR and LR in stack */
	b	.smc_exit

.smc_ret_to_nsec:
	/*
	 * When doing a return from secure world to non-secure world
	 * secure world has issued an smc with SMC_CALL_RETURN in r0
	 * and the return values for non-secure world in r1-r4. Non-secure
	 * world expects return values in r0-r3, do that shift here.
	 */
	pop	{r0}			/* smc call number should be removed */
	pop	{r0-r2}
	mov	r3, r4
	push	{r0-r3}

	/* Save secure context */
	bl	sm_get_sec_ctx
	stm	r0!, {r8-r11}
	bl	sm_save_modes_regs
	add	r1, sp, #SMC_ENTRY_SRS_OFFS /* Where srsdb wrote */
	ldm	r1, {r2-r3}		/* Load SPSR and LR from stack */
	stm	r0, {r2-r3}		/* Store SPSR and LR in context */

	/* Restore non-secure context */
	bl	sm_get_nsec_ctx
	ldm	r0!, {r4-r11}
	bl	sm_restore_modes_regs
	ldm	r0, {r2-r3}		/* Load SPSR and LR from context */
	add	r1, sp, #SMC_ENTRY_SRS_OFFS /* Where rfefd will read */
	stm	r1, {r2-r3}		/* Store SPSR and LR in stack */

	/* Set NS bit in SCR */
	mrc	p15, 0, r0, c1,c1, 0
	orr	r0, r0, #SCR_NS
	mcr	p15, 0, r0, c1,c1, 0

	b	.smc_exit

.smc_nsec_entry:
	/* Clear NS bit in SCR */
	bic	r1, r1, #SCR_NS
	mcr	p15, 0, r1, c1,c1, 0

	/* Save non-secure context */
	bl	sm_get_nsec_ctx
	stm	r0!, {r4-r11}
	bl	sm_save_modes_regs
	add	r1, sp, #SMC_ENTRY_SRS_OFFS /* Where srsdb wrote */
	ldm	r1, {r2-r3}		/* Load SPSR and LR from stack */
	stm	r0, {r2-r3}		/* Store SPSR and LR in context */

	/* Restore secure context */
	bl	sm_get_sec_ctx
	ldm	r0!, {r8-r11}
	bl	sm_restore_modes_regs
	ldm	r0, {r2-r3}		/* Load SPSR and LR from context */
	add	r1, sp, #SMC_ENTRY_SRS_OFFS /* Where rfefd will read */
	stm	r1, {r2-r3}		/* Store SPSR and LR in stack */

.smc_exit:
	pop	{r0-r3}
	rfefd	sp!
END_FUNC sm_smc_entry

/*
 * FIQ handling
 *
 * Saves CPU context in per core structure sm_pre_fiq_ctx which
 * later will be restored in the smc handler when handling a return
 * from FIQ.
 */
LOCAL_FUNC sm_fiq_entry , :
	srsdb	sp!, #CPSR_MODE_MON
	push	{r0-r8, r12}
/* Positions relative to stack pointer */
#define FIQ_ENTRY_SRS_OFFS	(2 * 4 + FIQ_ENTRY_R0R12_OFFS)
#define FIQ_ENTRY_R0R12_OFFS	(10 * 4)

	bl      sm_get_pre_fiq_ctx

	mrc	p15, 0, r1, c1,c1, 0	/* Get SCR */
	stm	r0!, {r1}		/* Save SCR */

	add	r1, sp, #FIQ_ENTRY_R0R12_OFFS /* Where R0-R3 are on stack */
	ldm	r1!, {r4-r7}		/* Load saved R0-R3 */
	stm	r0!, {r4-r7}		/* Save R0-R3 */
	ldm	r1!, {r4-r8, r12}	/* Load saved R4-R8 and R12 */
	stm	r0!, {r4-r12}
	bl	sm_save_modes_regs
	add	r1, sp, #FIQ_ENTRY_SRS_OFFS /* Where srsdb wrote */
	ldm	r1, {r2-r3}		/* Load SPSR and LR from stack */
	stm	r0, {r2-r3}		/* Store SPSR and LR in context */

	/* Clear NS bit in SCR */
	mrc	p15, 0, r0, c1,c1, 0
	bic	r0, r0, #SCR_NS
	mcr	p15, 0, r0, c1,c1, 0

	/* Restore secure context */
	bl	sm_get_sec_ctx
	ldm	r0!, {r8-r11, lr}
	bl	sm_restore_modes_regs
	ldm	r0, {r2-r3}		/* Load SPSR and LR from context */
	add	r1, sp, #FIQ_ENTRY_SRS_OFFS /* Where rfefd will read */
	stm	r1, {r2-r3}		/* Store SPSR and LR in stack */

	ldr	r0, =SMC_CALL_HANDLE_FIQ /* Post the FIQ to Trusted OS */

	add	sp, sp, #FIQ_ENTRY_R0R12_OFFS /* Peal of what was pushed */
	rfefd	sp!
END_FUNC sm_fiq_entry

        .align	5
LOCAL_FUNC sm_vect_table , :
	b	.		/* Reset			*/
	b	.		/* Undefined instruction	*/
	b	sm_smc_entry	/* Secure monitor call		*/
	b	.		/* Prefetch abort		*/
	b	.		/* Data abort			*/
	b	.		/* Reserved			*/
	b	.		/* IRQ				*/
	b	sm_fiq_entry	/* FIQ				*/
END_FUNC sm_vect_table

/* void sm_init(); */
FUNC sm_init , :
	push	{r0, lr}
	bl	sm_get_sp	/* Get the stack pointer */

	/* Set monitor stack */
	mrs	r1, cpsr
	cps	#CPSR_MODE_MON
	mov	sp, r0
	msr	cpsr, r1

	/* Set monitor vector (MVBAR) */
	ldr	r0, =sm_vect_table
	write_mvbar r0

	/* Configure FIQ exceptions to be taken to monitor mode. */
	read_scr r0
	orr	r0, r0, #SCR_FIQ
	write_scr r0

	pop	{r0, pc}
END_FUNC sm_init
