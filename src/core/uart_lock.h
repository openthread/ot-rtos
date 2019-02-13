/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef OT_FREERTOS_UART_LOCK_H_
#define OT_FREERTOS_UART_LOCK_H_

#include <openthread/openthread-freertos.h>
#include "openthread/platform/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function returns uart lock
 *
 * @returns OT_ERROR_NONE on success, error code on failure
 *
 */
otError otrUartLockInit(void);

/**
 * This function aquires uart lock
 *
 * @returns OT_ERROR_NONE on success, error code on failure
 *
 */
otError otCliUartLock(void);

/**
 * This function releases uart lock
 *
 * @returns OT_ERROR_NONE on success, error code on failure
 *
 */
otError otCliUartUnlock(void);

#define OT_CLI_UART_OUTPUT_LOCK() otCliUartLock()
#define OT_CLI_UART_OUTPUT_UNLOCK() otCliUartUnlock()

#ifdef __cplusplus
}
#endif

#endif // OT_FREERTOS_UART_LOCK_H_
