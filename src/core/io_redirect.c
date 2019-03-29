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
 *   This file implements write function for printf in arm newlibc
 */

#include <errno.h>

#include <openthread/cli.h>
#include <openthread/openthread-freertos.h>
#include <openthread/platform/uart.h>

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#if PLATFORM_nrf52 && (USB_CDC_AS_SERIAL_TRANSPORT == 1)

#include "platform-nrf5.h"
#include <drivers/clock/nrf_drv_clock.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_uart.h>

static bool sUartEnabled = false;

void debugUartInit()
{
    nrf_gpio_pin_set(UART_PIN_TX);
    nrf_gpio_cfg_output(UART_PIN_TX);
    nrf_gpio_cfg_input(UART_PIN_RX, NRF_GPIO_PIN_NOPULL);
    nrf_uart_txrx_pins_set(UART_INSTANCE, UART_PIN_TX, UART_PIN_RX);

    nrf_uart_configure(UART_INSTANCE, UART_PARITY, NRF_UART_HWFC_DISABLED);
    nrf_uart_baudrate_set(UART_INSTANCE, UART_BAUDRATE);

    nrf_uart_event_clear(UART_INSTANCE, NRF_UART_EVENT_TXDRDY);

    nrf_drv_clock_hfclk_request(NULL);
    while (!nrf_drv_clock_hfclk_is_running())
        ;

    nrf_uart_enable(UART_INSTANCE);
    sUartEnabled = true;
}

void debugUartputc(char c)
{
    nrf_uart_txd_set(UART_INSTANCE, c);
    nrf_uart_task_trigger(UART_INSTANCE, NRF_UART_TASK_STARTTX);
    while (!nrf_uart_event_check(UART_INSTANCE, NRF_UART_EVENT_TXDRDY))
        ;
    nrf_uart_event_clear(UART_INSTANCE, NRF_UART_EVENT_TXDRDY);
    nrf_uart_task_trigger(UART_INSTANCE, NRF_UART_TASK_STOPTX);
}

void debugUartPuts(const char *aStr, size_t aLength)
{
    if (!sUartEnabled)
    {
        debugUartInit();
    }
    for (size_t i = 0; i < aLength; i++)
    {
        debugUartputc(aStr[i]);
    }
}

#else

void debugUartPuts(const char *aStr, size_t aLength)
{
    otCliOutput(aStr, aLength);
}

#endif // PLATFROM_nrf52 && USB_CDC_AS_SERIAL_TRANSPORT == 1

#if !PLATFORM_linux

int _write(int aFile, const void *aStr, size_t aLength)
{
    int ret = aLength;

    if (aFile == STDOUT_FILENO)
    {
        otCliOutput(aStr, aLength);
    }
    else if (aFile == STDERR_FILENO)
    {
        debugUartPuts(aStr, aLength);
    }
    else
    {
        errno = EBADF;
        ret   = -1;
    }

    return ret;
}

#endif

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_APP
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);

    va_list ap;
    va_start(ap, aFormat);
    otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
    va_end(ap);
}
#endif
