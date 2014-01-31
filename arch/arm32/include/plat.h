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
#ifndef PLAT_H
#define PLAT_H

#define STACK_TMP_SIZE		256	/* TODO minimize */
#define STACK_ABT_SIZE		1024
#define STACK_IRQ_SIZE		256	/* TODO minimize */
#define STACK_THREAD_SIZE	(8 * 1024)
#define STACK_ALIGMENT		8

#define MMU_L1_NUM_ENTRIES	4096		/* Maps 4 GiB */
#define MMU_L1_ALIGNMENT	(1 << 14)	/* 16 KiB aligned */

#define GIC_BASE                0x2c000000
#define GICC_OFFSET             0x2000
#define GICD_OFFSET             0x1000

#define UART0_BASE		0x1c090000
#define UART1_BASE		0x1c0a0000
#define UART2_BASE		0x1c0b0000
#define UART3_BASE		0x1c0c0000

#define DDR0_BASE		0x80000000
#define DDR0_SIZE		(510 * 1024 * 1024)

#endif /*PLAT_H*/
