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
#include <kern/mmu.h>
#include <sm/sm_defs.h>
#include <tee/entry.h>
#include <kprintf.h>

#define TEE_CALL(func_num)	SMC_CALL_VAL(SMC_STD_CALL, SMC_32, \
					SMC_OWNER_TRUSTED_OS, (func_num))

#define TEE_FUNC_OPEN_SESSION    TEE_CALL(1)
#define TEE_FUNC_CLOSE_SESSION   TEE_CALL(2)
#define TEE_FUNC_INVOKE          TEE_CALL(3)
#define TEE_FUNC_REGISTER_RPC    TEE_CALL(4)
#define TEE_FUNC_CANCEL          TEE_CALL(5)

#define TEEC_CONFIG_PAYLOAD_REF_COUNT 4
#define TEE_UUID_CLOCK_SIZE 8


struct tee_uuid {
	uint32_t timeLow;
	uint16_t timeMid;
	uint16_t timeHiAndVersion;
	uint8_t clockSeqAndNode[TEE_UUID_CLOCK_SIZE];
};

struct tee_identity {
	uint32_t login;
	struct tee_uuid uuid;
};

struct tee_value {
	uint32_t a;
	uint32_t b;
};


struct tee_memref {
	void *buffer;
	size_t size;
};

struct tee_open_session_arg {
	uint32_t res;
	uint32_t origin;
	uint32_t sess;
	struct ta_signed_header_t *ta;
	struct tee_uuid *uuid;
	uint32_t param_types;
	struct tee_value params[TEEC_CONFIG_PAYLOAD_REF_COUNT];
	struct tee_identity client_id;
	uint32_t params_flags[TEEC_CONFIG_PAYLOAD_REF_COUNT];
};

struct tee_invoke_command_arg {
	uint32_t res;
	uint32_t origin;
	uint32_t sess;
	uint32_t cmd;
	uint32_t param_types;
	struct tee_value params[TEEC_CONFIG_PAYLOAD_REF_COUNT];
	struct tee_identity client_id;
	uint32_t params_flags[TEEC_CONFIG_PAYLOAD_REF_COUNT];
};

struct tee_cancel_command_arg {
	uint32_t res;
	uint32_t origin;
	uint32_t sess;
	struct tee_identity client_id;
};

static void tee_invoke(struct tee_invoke_command_arg *arg)
{
	arg->res = 0;
	arg->origin = 0;
	arg->params[0].a = arg->params[0].a +  arg->params[0].b;
}

void tee_entry(struct thread_smc_args *args)
{
	switch (args->smc_func_id) {
	case TEE_FUNC_OPEN_SESSION:
		kprintf("TEE_FUNC_OPEN_SESSION\n");
		args->smc_func_id = 0;
		break;
	case TEE_FUNC_CLOSE_SESSION:
		kprintf("TEE_FUNC_CLOSE_SESSION\n");
		args->smc_func_id = -1;
		break;
	case TEE_FUNC_INVOKE:
		kprintf("TEE_FUNC_INVOKE\n");
		tee_invoke((struct tee_invoke_command_arg *)args->param1);
		args->smc_func_id = 0;
		break;
	case TEE_FUNC_REGISTER_RPC:
		kprintf("TEE_FUNC_REGISTER_RPC\n");
		args->smc_func_id = -1;
		break;
	case TEE_FUNC_CANCEL:
		kprintf("TEE_FUNC_CANCEL\n");
		args->smc_func_id = -1;
		break;
	default:
		kprintf("Unknown function 0x%x\n", args->smc_func_id);
		kprintf("TEE_FUNC_OPEN_SESSION  0x%x\n", TEE_FUNC_OPEN_SESSION);
		kprintf("TEE_FUNC_CLOSE_SESSION 0x%x\n",
			TEE_FUNC_CLOSE_SESSION);
		kprintf("TEE_FUNC_INVOKE        0x%x\n", TEE_FUNC_INVOKE);
		kprintf("TEE_FUNC_REGISTER_RPC  0x%x\n", TEE_FUNC_REGISTER_RPC);
		kprintf("TEE_FUNC_CANCEL        0x%x\n", TEE_FUNC_CANCEL);
		args->smc_func_id = -1;
		break;;
	}
}
