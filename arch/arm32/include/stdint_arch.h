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
#ifndef STDINT_ARCH_H
#define STDINT_ARCH_H

/* 7.18.1.1 Exact-width interger types */

typedef signed char int8_t;
typedef short int16_t;
typedef unsigned int int32_t;
typedef long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/* 7.18.1.4 Integer types capable of holding object pointers */

typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

/* 7.18.1.5 Greatest-width integer types */

typedef uint64_t uintmax_t;
typedef int64_t intmax_t;


/* 7.18.2.4 Limits of integer types capable of holding object pointers */

#define INTPTR_MIN	INT32_MIN
#define INTPTR_MAX	INT32_MAX
#define UINTPTR_MAX	UINT32_MAX

/* 7.18.2.5 Limits of greatest-width integer types */

#define UINTMAX_MAX	UINT64_MAX
#define INTMAX_MAX	INT64_MAX
#define INTMAX_MIN	INT64_MIN

#endif /*STDINT_ARCH_H*/
