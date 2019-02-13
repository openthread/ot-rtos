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

/**
 * @file
 *   This file implements porting of LwIP on FreeRTOS.
 */

#include <assert.h>

#include <arch/cc.h>
#include <arch/sys_arch.h>

#include <lwip/debug.h>
#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/stats.h>
#include <lwip/sys.h>

#include <stdbool.h>

static sys_mutex_t g_lwip_protect_mutex = NULL;

#if !LWIP_COMPAT_MUTEX
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    err_t err = ERR_MEM;

    *mutex = xSemaphoreCreateMutex();

    if (*mutex != NULL)
    {
        err = ERR_OK;
    }

    return err;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
    while (xSemaphoreTake(*mutex, portMAX_DELAY) != pdPASS)
        ;
}

err_t sys_mutex_trylock(sys_mutex_t *mutex)
{
    if (xSemaphoreTake(*mutex, 0) == pdPASS)
        return 0;
    else
        return -1;
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
    xSemaphoreGive(*mutex);
}

void sys_mutex_free(sys_mutex_t *mutex)
{
    vQueueDelete(*mutex);
}
#endif

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    err_t err = ERR_MEM;
    *sem      = xSemaphoreCreateBinary();

    if ((*sem) != NULL)
    {
        if (count == 0)
        {
            xSemaphoreTake(*sem, 1);
        }

        err = ERR_OK;
    }

    return err;
}

void sys_sem_signal(sys_sem_t *sem)
{
    xSemaphoreGive(*sem);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    portTickType  StartTime, EndTime, Elapsed;
    unsigned long ret;

    StartTime = xTaskGetTickCount();

    if (timeout != 0)
    {
        if (xSemaphoreTake(*sem, timeout / portTICK_PERIOD_MS) == pdTRUE)
        {
            EndTime = xTaskGetTickCount();
            Elapsed = (EndTime - StartTime) * portTICK_PERIOD_MS;

            ret = Elapsed;
        }
        else
        {
            ret = SYS_ARCH_TIMEOUT;
        }
    }
    else
    {
        while (xSemaphoreTake(*sem, portMAX_DELAY) != pdTRUE)
            ;

        EndTime = xTaskGetTickCount();
        Elapsed = (EndTime - StartTime) * portTICK_PERIOD_MS;

        ret = Elapsed;
    }

    return ret;
}

void sys_sem_free(sys_sem_t *sem)
{
    vSemaphoreDelete(*sem);
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    *mbox = mem_malloc(sizeof(struct sys_mbox_s));
    if (*mbox == NULL)
    {
        return ERR_MEM;
    }

    (*mbox)->os_mbox = xQueueCreate(size, sizeof(void *));

    if ((*mbox)->os_mbox == NULL)
    {
        mem_free(*mbox);
        return ERR_MEM;
    }

    if (sys_mutex_new(&((*mbox)->lock)) != ERR_OK)
    {
        vQueueDelete((*mbox)->os_mbox);
        mem_free(*mbox);
        return ERR_MEM;
    }

    (*mbox)->alive = true;

    return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    while (xQueueSendToBack((*mbox)->os_mbox, &msg, portMAX_DELAY) != pdTRUE)
        ;
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    err_t err;

    if (xQueueSend((*mbox)->os_mbox, &msg, (portTickType)0) == pdPASS)
    {
        err = ERR_OK;
    }
    else
    {
        err = ERR_MEM;
    }

    return err;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    err_t      err;
    BaseType_t xHigherPriorityTaskWoken;

    if (xQueueSendFromISR((*mbox)->os_mbox, &msg, &xHigherPriorityTaskWoken) == pdPASS)
    {
        err = ERR_OK;
    }
    else
    {
        err = ERR_MEM;
    }

    if (xHigherPriorityTaskWoken)
    {
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }

    return err;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    void *        dummyptr;
    portTickType  StartTime, EndTime, Elapsed;
    unsigned long ret;

    StartTime = xTaskGetTickCount();
    if (msg == NULL)
    {
        msg = &dummyptr;
    }

    if (*mbox == NULL)
    {
        *msg = NULL;
        return -1;
    }

    sys_mutex_lock(&(*mbox)->lock);

    if (timeout != 0)
    {
        if (pdTRUE == xQueueReceive((*mbox)->os_mbox, &(*msg), timeout / portTICK_PERIOD_MS))
        {
            EndTime = xTaskGetTickCount();
            Elapsed = (EndTime - StartTime) * portTICK_PERIOD_MS;

            if (Elapsed == 0)
            {
                Elapsed = 1;
            }

            ret = Elapsed;
        }
        else
        {
            *msg = NULL;
            ret  = SYS_ARCH_TIMEOUT;
        }
    }
    else
    {
        while (1)
        {
            if (pdTRUE == xQueueReceive((*mbox)->os_mbox, &(*msg), portMAX_DELAY))
            {
                break;
            }

            if ((*mbox)->alive == false)
            {
                *msg = NULL;
                break;
            }
        }

        EndTime = xTaskGetTickCount();
        Elapsed = (EndTime - StartTime) * portTICK_PERIOD_MS;

        if (Elapsed == 0)
        {
            Elapsed = 1;
        }

        ret = Elapsed;
    }

    sys_mutex_unlock(&(*mbox)->lock);

    return ret;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    void *        dummy;
    unsigned long ret;

    if (msg == NULL)
    {
        msg = &dummy;
    }

    if (pdTRUE == xQueueReceive((*mbox)->os_mbox, &(*msg), 0))
    {
        ret = ERR_OK;
    }
    else
    {
        ret = SYS_MBOX_EMPTY;
    }

    return ret;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
#define MAX_POLL_CNT 100
#define PER_POLL_DELAY 20
    uint16_t count     = 0;
    bool     post_null = true;

    (*mbox)->alive = false;

    while (count++ < MAX_POLL_CNT)
    {
        if (!sys_mutex_trylock(&(*mbox)->lock))
        {
            sys_mutex_unlock(&(*mbox)->lock);
            break;
        }

        if (post_null)
        {
            if (sys_mbox_trypost(mbox, NULL) != ERR_OK)
            {
            }
            else
            {
                post_null = false;
            }
        }

        if (count == (MAX_POLL_CNT - 1))
        {
            printf("WARNING: mbox %p had a consumer who never unblocked. Leaking!\n", (*mbox)->os_mbox);
        }
        vTaskDelay(PER_POLL_DELAY / portTICK_PERIOD_MS);
    }

    if (uxQueueMessagesWaiting((*mbox)->os_mbox))
    {
        xQueueReset((*mbox)->os_mbox);
        /* Line for breakpoint.  Should never break here! */
        __asm__ volatile("nop");
    }

    vQueueDelete((*mbox)->os_mbox);
    sys_mutex_free(&(*mbox)->lock);
    mem_free(*mbox);
    *mbox = NULL;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    xTaskHandle   CreatedTask;
    portBASE_TYPE result;

    result = xTaskCreate(thread, name, stacksize, arg, prio, &CreatedTask);

    if (result == pdPASS)
    {
        return CreatedTask;
    }
    else
    {
        return NULL;
    }
}

void sys_init(void)
{
    if (ERR_OK != sys_mutex_new(&g_lwip_protect_mutex))
    {
        printf("sys_init: failed to init lwip protect mutex\n");
    }
}

sys_prot_t sys_arch_protect(void)
{
    sys_mutex_lock(&g_lwip_protect_mutex);
    return (sys_prot_t)1;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    (void)pval;
    sys_mutex_unlock(&g_lwip_protect_mutex);
}

u32_t sys_now(void)
{
    return (xTaskGetTickCount() * portTICK_PERIOD_MS);
}
