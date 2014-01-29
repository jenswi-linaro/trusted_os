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
#include <stdbool.h>
#include <stddef.h>

#include <arm32.h>
#include <kern/mmu.h>
#include <kern/cache.h>

#define MMU_L1_TYPE_WBWA \
	((0x1 << MMU_L1_TEX_SHIFT) | MMU_L1_B | MMU_L1_C)
#define MMU_L1_TYPE_DEVICE \
	((0x0 << MMU_L1_TEX_SHIFT) | MMU_L1_B)

#define MMU_L1_TEX_SHIFT	12

#define MMU_L1_PAGE_TBL		0x1
#define MMU_L1_SECTION		0x2
#define MMU_L1_B		(1 << 2)
#define MMU_L1_C		(1 << 3)
#define MMU_L1_XN		(1 << 4)
#define MMU_L1_AP0		(1 << 10)
#define MMU_L1_AP1		(1 << 11)
#define MMU_L1_AP2		(1 << 15)
#define MMU_L1_S		(1 << 16)
#define MMU_L1_NG		(1 << 17)
#define MMU_L1_NS		(1 << 19)

#define MMU_SMALL_PAGE_SHIFT	12
#define MMU_SMALL_PAGE_MASK	0xfff
#define MMU_SMALL_PAGE_SIZE	0x1000

#define MMU_SECTION_SHIFT	20
#define MMU_SECTION_MASK	0x000fffff
#define MMU_SECTION_SIZE	0x00100000

/* Sharable */
#define MMU_TTBR_S		(1 << 1)
/* Normal memory, Inner Write-Back Write-Allocate Cacheable */
#define MMU_TTBR_IRGN_WBWA	(1 << 6)
/* Normal memory, Outer Write-Back Write-Allocate Cacheable */
#define MMU_TTBR_RNG_WBWA	(1 << 3)


#define MMU_TTBR_SHARED_WBWA \
	(MMU_TTBR_S | MMU_TTBR_IRGN_WBWA | MMU_TTBR_RNG_WBWA)


static struct {
	uint32_t *l1_table;
} mmu __attribute__((section(".bss.prebss.mmu")));

static uint32_t create_romem_block(uintptr_t addr, bool ns)
{
	uint32_t attrs;

	attrs = MMU_L1_TYPE_WBWA |
		MMU_L1_SECTION |
		MMU_L1_S |	/* shared, global */
		MMU_L1_AP2 |	/* RO PL1, other levels no access */
		MMU_L1_AP0;	/* Accessable */

	if (ns)
		attrs |= MMU_L1_NS;

	return (addr & ~MMU_SECTION_MASK) | attrs;
}

static uint32_t create_rwmem_block(uintptr_t addr, bool ns)
{
	uint32_t attrs;

	attrs = MMU_L1_TYPE_WBWA |
		MMU_L1_SECTION |
		MMU_L1_S |	/* shared, global */
		0 |		/* RW PL1, other levels no access */
		MMU_L1_AP0;	/* Accessable */

	if (ns)
		attrs |= MMU_L1_NS;

	return (addr & ~MMU_SECTION_MASK) | attrs;
}

static uint32_t create_device_block(uintptr_t addr, bool ns)
{
	uint32_t attrs;

	attrs = MMU_L1_TYPE_DEVICE |
		MMU_L1_SECTION |
		MMU_L1_S |	/* shared, global */
		MMU_L1_XN |	/* Not executable */
		0 |		/* RW PL1, other levels no access */
		MMU_L1_AP0;	/* Accessable */

	if (ns)
		attrs |= MMU_L1_NS;

	return (addr & ~MMU_SECTION_MASK) | attrs;
}




void mmu_init(uint32_t *l1_table, uintptr_t code_start, uintptr_t code_end,
	uintptr_t data_start, uintptr_t data_end)
{
	size_t n;
	uint32_t sctlr;
	uintptr_t a;

	mmu.l1_table = l1_table;

	for (n = 0; n < 4096; n++)
		mmu.l1_table[n] = 0;

	/* Idenity map code */
	for (a = code_start & ~MMU_SECTION_MASK; a < code_end;
			a += MMU_SECTION_SIZE) {
		mmu.l1_table[a >> MMU_SECTION_SHIFT] =
			create_romem_block(a, false);
	}

	/* Idenity map data */
	for (a = data_start & ~MMU_SECTION_MASK; a < data_end;
			a += MMU_SECTION_SIZE) {
		mmu.l1_table[a >> MMU_SECTION_SHIFT] =
			create_rwmem_block(a, false);
	}

	write_ttbr0((uint32_t)l1_table | MMU_TTBR_SHARED_WBWA);

	write_dacr(1);

	sctlr = read_sctlr();
	sctlr |= SCTLR_AFE;	/* Simplified access permissions */
	sctlr |= SCTLR_M;	/* Enable MMU */
	sctlr |= SCTLR_C;	/* Enable data cache */
	sctlr |= SCTLR_I;	/* Enable instruction cache */
	sctlr |= SCTLR_Z;	/* Enable branch prediction */
	write_sctlr(sctlr);
	isb();
}

vaddr_t mmu_map_device(paddr_t addr, size_t len)
{
	paddr_t a;
	bool inv_needed = false;

	for (a = addr & ~MMU_SECTION_MASK; a < (addr + len);
			a += MMU_SECTION_SIZE) {
		uint32_t entry = create_device_block(addr, false);

		if (mmu.l1_table[a >> MMU_SECTION_SHIFT] != entry) {
			mmu.l1_table[a >> MMU_SECTION_SHIFT] =
				create_device_block(a, false);
			inv_needed = true;
		}
	}
	/* TODO only invalidate range */
	if (inv_needed)
		cache_tlb_invalidate();

	return addr;
}

vaddr_t mmu_map_rwmem(paddr_t addr, size_t len, bool ns)
{
	paddr_t a;
	bool inv_needed = false;

	for (a = addr & ~MMU_SECTION_MASK; a < (addr + len);
			a += MMU_SECTION_SIZE) {
		uint32_t entry = create_rwmem_block(addr, ns);

		if (mmu.l1_table[a >> MMU_SECTION_SHIFT] != entry) {
			mmu.l1_table[a >> MMU_SECTION_SHIFT] =
				create_device_block(a, ns);
			inv_needed = true;
		}
	}
	/* TODO only invalidate range */
	if (inv_needed)
		cache_tlb_invalidate();

	return addr;
}
