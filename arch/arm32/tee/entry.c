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
#include <sm/teesmc.h>
#include <tee/entry.h>
#include <string.h>
#include <kprintf.h>
#include <assert.h>

static void tee_invoke(struct teesmc32_arg *arg32)
{
	union teesmc32_param *params = TEESMC32_GET_PARAMS(arg32);
	struct teesmc32_arg *smcarg;
	paddr_t phsmcarg;

	kprintf("Doing thread_rpc_alloc\n");
	thread_rpc_alloc(sizeof(struct teesmc32_arg), 0, &phsmcarg, NULL);
	kprintf("thread_rpc_alloc returned 0x%x\n", phsmcarg);
	smcarg = (struct teesmc32_arg *)phsmcarg;
	memset(smcarg, 0, sizeof(struct teesmc32_arg));
	smcarg->cmd = 0x12345;

	kprintf("Doing RPC cmd 0x%x (phsmcarg 0x%x)\n", smcarg->cmd, phsmcarg);

	thread_rpc_cmd(phsmcarg);

	kprintf("RPC returned 0x%x\n", smcarg->ret);

	kprintf("Doing thread_rpc_free\n");
	thread_rpc_free(phsmcarg, 0);
	kprintf("thread_rpc_free returned\n");

	assert(arg32->num_params > 0);

	arg32->ret = 0;
	arg32->ret_origin = 0;
	params[0].value.a = params[0].value.a + params[0].value.b;
}

void tee_entry(struct thread_smc_args *args)
{
	struct teesmc32_arg *arg32;

	if (args->a0 != TEESMC32_CALL_WITH_ARG &&
	    args->a0 != TEESMC32_FASTCALL_WITH_ARG) {
		kprintf("Unknown SMC 0x%x\n", args->a0);
		kprintf("Expected 0x%x or 0x%x\n",
			TEESMC32_CALL_WITH_ARG, TEESMC32_FASTCALL_WITH_ARG);
		args->a0 = -1;
		return;
	}

	arg32 = (struct teesmc32_arg *)args->a1;

	switch (arg32->cmd) {
	case TEESMC_CMD_OPEN_SESSION:
		kprintf("TEESMC_CMD_OPEN_SESSION\n");
		args->a0 = TEESMC_RETURN_OK;
		break;
	case TEESMC_CMD_CLOSE_SESSION:
		kprintf("TEESMC_CMD_CLOSE_SESSION\n");
		args->a0 = TEESMC_RETURN_OK;
		break;
	case TEESMC_CMD_INVOKE_COMMAND:
		kprintf("TEESMC_CMD_INVOKE_COMMAND\n");
		tee_invoke(arg32);
		args->a0 = TEESMC_RETURN_OK;
		break;
	case TEESMC_CMD_CANCEL:
		kprintf("TEESMC_CMD_CANCEL\n");
		args->a0 = TEESMC_RETURN_OK;
		break;
	default:
		kprintf("Unknown cmd 0x%x\n", arg32->cmd);
		args->a0 = TEESMC_RETURN_UNKNOWN_FUNCTION;
		break;;
	}
}
