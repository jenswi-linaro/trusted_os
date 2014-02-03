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
#ifndef SM_DEFS_H
#define SM_DEFS_H


#define SMC_32			0
#define SMC_64			0x40000000
#define SMC_FAST_CALL		0x80000000
#define	SMC_STD_CALL		0

#define SMC_OWNER_MASK		0x3F
#define SMC_OWNER_SHIFT		24

#define SMC_FUNC_MASK		0xFFFF

#define	SMC_IS_FAST_CALL(smc_val)	((smc_val) & SMC_FAST_CALL)
#define SMC_IS_64(smc_val)		((smc_val) & SMC_64)
#define	SMC_FUNC_NUM(smc_val)		((smc_val) & SMC_FUNC_MASK)
#define	SMC_OWNER_NUM(smc_val)		(((smc_val) >> SMC_OWNER_SHIFT) & \
					 SMC_OWNER_MASK)

#define SMC_CALL_VAL(type, calling_convention, owner, func_num) \
			((type) | (calling_convention) | \
			(((owner) & SMC_OWNER_MASK) << SMC_OWNER_SHIFT) | \
			((func_num) & SMC_FUNC_MASK))

#define SMC_OWNER_ARCH		0
#define SMC_OWNER_CPU		1
#define SMC_OWNER_SIP		2
#define SMC_OWNER_OEM		3
#define SMC_OWNER_STANDARD	4
#define SMC_OWNER_TRUSTED_APP	48
#define SMC_OWNER_TRUSTED_OS	50
#define SMC_OWNER_MONITOR	63


/* From Trusted OS to monitor, return from last call */
#define	SMC_CALL_RETURN		SMC_CALL_VAL(SMC_32, SMC_FAST_CALL, \
					     SMC_OWNER_MONITOR, 0)
/* From monitor to Trusted OS, handle FIQ. */
#define	SMC_CALL_HANDLE_FIQ	SMC_CALL_VAL(SMC_32, SMC_FAST_CALL, \
					     SMC_OWNER_MONITOR, 1)
/* From Trusted OS to monitor, return from FIQ. */
#define	SMC_CALL_RETURN_FROM_FIQ SMC_CALL_VAL(SMC_32, SMC_FAST_CALL, \
					     SMC_OWNER_MONITOR, 2)
/* From Trusted OS to monitor, request RPC (or IRQ). */
#define	SMC_CALL_REQUEST_RPC	SMC_CALL_VAL(SMC_32, SMC_FAST_CALL, \
					     SMC_OWNER_MONITOR, 3)
/* From monitor to Trusted OS, unknown monitor call from Trusted OS */
#define	SMC_CALL_UNKNOWN	SMC_CALL_VAL(SMC_32, SMC_FAST_CALL, \
					     SMC_OWNER_MONITOR, 4)

/* From non-secure world to Trusted OS, resume from RPC (or IRQ) */
#define	SMC_CALL_RETURN_FROM_RPC SMC_CALL_VAL(SMC_32, SMC_STD_CALL, \
					     SMC_OWNER_TRUSTED_OS, 0)

/* Returned in r0 */
#define SMC_RETURN_UNKNOWN_FUNCTION	0xFFFFFFFF

/* Returned in r0 only from Trusted OS functions */
#define SMC_RETURN_TRUSTED_OS_OK	0x0
#define SMC_RETURN_TRUSTED_OS_RPC	0x1
#define SMC_RETURN_TRUSTED_OS_ENOTHR	0x10
#define SMC_RETURN_TRUSTED_OS_EBADTHR	0x11

/* Returned in r1 by Trusted OS functions if r0 = SMC_RETURN_TRUSTED_OS_RPC */
#define SMC_RPC_REQUEST_IRQ		0x0

#if 0
/*
 * sm_smc_entry uses 6 * 4 bytes
 * sm_fiq_entry uses 12 * 4 bytes
 */
#define SM_STACK_SIZE	(12 * 4)
#else
/* TODO optimize sm_get_pre_fiq_ctx() */
#define SM_STACK_SIZE	(24 * 4)
#endif

#endif /*SM_DEFS_H*/
