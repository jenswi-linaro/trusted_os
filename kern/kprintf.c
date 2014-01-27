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
#include <kprintf.h>

static struct {
	kvprintf_putc putc;
	kprintf_flush_output flush_output;
	void *arg;
} kprintf_data __attribute__((section(".bss.prebss.kprintf")));

void kprintf_init(kvprintf_putc putc, kprintf_flush_output flush_output,
		void *arg)
{
	kprintf_data.putc = putc;
	kprintf_data.flush_output = flush_output;
	kprintf_data.arg = arg;
}

static void output(int ch, void *arg)
{
	kprintf_data.putc(ch, arg);
	if (ch == '\n') {
		kprintf_data.putc('\r', arg);
		kprintf_data.flush_output(arg);
	}
}

void kprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	kvprintf(output, kprintf_data.arg, 10, fmt, ap);
	va_end(ap);
}
