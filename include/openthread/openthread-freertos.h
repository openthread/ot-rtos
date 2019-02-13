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

/**
 * @file
 *   This file includes interface definitions for FreeRTOS.
 *
 */

#ifndef OPENTHREAD_FREERTOS
#define OPENTHREAD_FREERTOS

#include <FreeRTOS.h>
#include <portmacro.h>
#include <openthread/instance.h>

#include "portable/portable.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function initializes FreeRTOS and OpenThread.
 *
 */
void otrInit(int argc, char *argv[]);

/**
 * This function starts OpenThread task.
 *
 */
void otrStart(void);

/**
 * This function notifies OpenThread task.
 *
 */
void otrTaskNotifyGive(void);

/**
 * This function notifies OpenThread task from ISR.
 *
 */
void otrTaskNotifyGiveFromISR(void);

/**
 * This function locks OpenThread task.
 *
 */
void otrLock(void);

/**
 * This function unlocks OpenThread task.
 */
void otrUnlock(void);

/**
 * This function initializes user application.
 */
void otrUserInit(void);

/**
 * This function gets ot instance
 */
otInstance *otrGetInstance();

#ifndef portFORCE_INLINE
#define portFORCE_INLINE inline __attribute__((always_inline))
#endif
/**
 * This function returns if it is in ISR context.
 *
 */
portFORCE_INLINE static BaseType_t otrPortIsInsideInterrupt(void)
{
    uint32_t   ulCurrentInterrupt;
    BaseType_t xReturn;

    /* Obtain the number of the currently executing interrupt. */
    OTR_PORT_GET_IN_ISR(ulCurrentInterrupt);

    if (ulCurrentInterrupt == 0)
    {
        xReturn = pdFALSE;
    }
    else
    {
        xReturn = pdTRUE;
    }

    return xReturn;
}

/**
 * This macro provides a thread-safe way to call OpenThread api in other threads
 *
 *  @param[in]  ...    function call statement of OpenThread api
 *
 */
#define OT_API_CALL(...)     \
    do                       \
    {                        \
        otrLock();           \
        __VA_ARGS__;         \
        otrUnlock();         \
        otrTaskNotifyGive(); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_FREERTOS
