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
#include <kern/resmem.h>
#include <kern/kern.h>

static struct {
	uintptr_t begin;
	uintptr_t end;
	bool disabled;
} resmem __attribute__((section(".bss.prebss.resmem")));

/* Alignment of returned pointers */
#define RESMEM_ALIGN	8

void resmem_init(uintptr_t begin, uintptr_t end)
{
	resmem.begin = ROUNDUP(begin, RESMEM_ALIGN);
	resmem.end = end;

	if (resmem.begin < begin)
		resmem.disabled = true;
	else
		resmem.disabled = false;
}

void *resmem_alloc(size_t size)
{
	size_t s = ROUNDUP(size, RESMEM_ALIGN);
	uintptr_t new_begin = resmem.begin + s;
	uintptr_t old_begin = resmem.begin;

	if (resmem.disabled || s < size || new_begin < resmem.begin ||
	    new_begin > resmem.end)
		return NULL;

	resmem.begin = new_begin;
	return (void *)old_begin;
}

void resmem_disable(void)
{
	resmem.disabled = true;
}

void resmem_get_limits(uintptr_t *begin, uintptr_t *end)
{
	*begin = resmem.begin;
	*end = resmem.end;
}
