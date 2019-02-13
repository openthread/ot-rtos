/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef xSemaphoreHandle sys_sem_t;
typedef xSemaphoreHandle sys_mutex_t;
typedef xTaskHandle      sys_thread_t;

typedef struct sys_mbox_s
{
    xQueueHandle os_mbox;
    sys_mutex_t  lock;
    uint8_t      alive;
} * sys_mbox_t;

#define LWIP_COMPAT_MUTEX 0

#if !LWIP_COMPAT_MUTEX
#define sys_mutex_valid(x) (((*x) == NULL) ? pdFALSE : pdTRUE)
#define sys_mutex_set_invalid(x) ((*x) = NULL)
#endif

#define sys_mbox_valid(x) (((*x) == NULL) ? pdFALSE : pdTRUE)
#define sys_mbox_set_invalid(x) ((*x) = NULL)

#define sys_sem_valid(x) (((*x) == NULL) ? pdFALSE : pdTRUE)
#define sys_sem_set_invalid(x) ((*x) = NULL)

typedef uint32_t sys_prot_t;

#ifdef __cplusplus
}
#endif

#endif /* __SYS_ARCH_H__ */
