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
#include <kern/mutex.h>

FUNC __mutex_lock , :
	mov	r1, #MUTEX_LOCKED
.lock_retry:
	ldrex	r2, [r0]
	cmp	r2, r1		/* Test if mutex is locked or unlocked */
	beq	.lock_retry	/* If locked try again */
	strexne	r2, r1, [r0]	/* Not locked, attempt to lock it */
	cmpne	r2, #MUTEX_LOCKED
	beq	.lock_retry	/* Failed, retry */
	/* Lock acquired */
	dmb			/* Req before accessing protected resource */
	bx	lr
END_FUNC __mutex_lock

FUNC __mutex_trylock , :
	mov	r1, #MUTEX_LOCKED
	ldrex	r2, [r0]
	cmp	r2, r1		/* Test if mutex is locked or unlocked */
	beq	.trylock_fail	/* If not locked return */
	strexne	r2, r1, [r0]	/* Not locked, attempt to lock it */
	cmpne	r2, #MUTEX_LOCKED
	beq	.trylock_fail	/* Failed, return */
	/* Lock acquired */
	dmb			/* Req before accessing protected resource */
	mov	r0, #1
	bx	lr

.trylock_fail:
	mov	r0, #0
	bx	lr
END_FUNC __mutex_trylock

FUNC __mutex_unlock , :
	mov	r1, #MUTEX_UNLOCKED
	dmb			/* Req before releasing protected resource */
	str	r1, [r0]	/* Unlock mutex */
	bx	lr
END_FUNC __mutex_unlock
