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
#include <drivers/gic.h>
#include <io.h>
#include <kern/mmu.h>

/* Offsets from gic.gicc_base */
#define GICC_CTLR		0

#define GICC_CTLR_ENABLEGRP0	(1 << 0)
#define GICC_CTLR_ENABLEGRP1	(1 << 1)
#define GICC_CTLR_FIQEN		(1 << 3)

/* Offsets from gic.gicd_base */
#define GICD_CTLR		0
#define GICD_ICENABLER(n)       (0x180 + (n) * 4)
#define GICD_ICPENDR(n)         (0x280 + (n) * 4)
#define GICD_IGROUPR(n)         (0x080 + (n) * 4)



static struct {
	vaddr_t gicc_base;
	vaddr_t gicd_base;
	size_t max_ints;
} gic;

void gic_init(vaddr_t gicc_base, vaddr_t gicd_base)
{
	size_t n;

	gic.gicc_base = gicc_base;
	gic.gicd_base = gicd_base;
	gic.max_ints = 3 * 32; /* TODO probe this */

	for (n = 0; n < gic.max_ints / 32; n++) {
		/* Disable interrupts */
		write32(-1, gic.gicd_base + GICD_ICENABLER(n));

		/* Make interrupts non-pending */
		write32(-1, gic.gicd_base + GICD_ICPENDR(n));

		/* Mark interrupts non-secure */
		write32(-1, gic.gicd_base + GICD_IGROUPR(n));
	}

	/* Enable GIC */
	write32(GICC_CTLR_ENABLEGRP0 | GICC_CTLR_ENABLEGRP1 | GICC_CTLR_FIQEN,
		gic.gicc_base + GICC_CTLR);
}
