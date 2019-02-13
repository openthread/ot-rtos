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

#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <openthread/ip6.h>
#include <openthread/joiner.h>
#include <openthread/openthread-freertos.h>
#include <openthread/thread.h>

#include <lwip/altcp_tcp.h>
#include <lwip/apps/http_client.h>

#include <nrfx/hal/nrf_gpio.h>
#include <nrfx/hal/nrf_gpiote.h>

#include "net/utils/nat64_utils.h"

#ifndef DEMO_PASSPHRASE
#define DEMO_PASSPHRASE "ABCDEF"
#endif

#define COMMISSION_BIT (1 << 1)
#define HTTP_BIT (1 << 2)
#define BUTTON_BIT (1 << 3)

#define BUTTON1_PIN 11
#define BUTTON2_PIN 12
#define LED1_PIN 13
#define GPIO_PRIORITY 6

static TaskHandle_t sDemoTask;

static void HandleJoinerCallback(otError aError, void *aContext)
{
    printf("Joiner callback with error\n", aError);
    if (aError == OT_ERROR_NONE)
    {
        xTaskNotify(sDemoTask, COMMISSION_BIT, eSetBits);
    }
}

static void WaitForSignal(uint32_t aSignal)
{
    uint32_t notifyValue = 0;

    do
    {
        xTaskNotifyWait(aSignal, aSignal, &notifyValue, portMAX_DELAY);
    } while ((notifyValue & aSignal) == 0);
}

static void HttpDoneCallback(void *aArg, httpc_result_t aResult, uint32_t aLen, uint32_t aStatusCode, err_t aErr)
{
    (void)aArg;
    (void)aResult;
    (void)aLen;

    printf("Status code %d err %d\n", aStatusCode, aErr);
    xTaskNotify(sDemoTask, HTTP_BIT, eSetBits);
}

static err_t HttpHeaderCallback(httpc_state_t *aConn,
                                void *         aArg,
                                struct pbuf *  aHdr,
                                uint16_t       aLen,
                                uint32_t       aContentLen)
{
    (void)aConn;
    (void)aArg;
    (void)aHdr;

    printf("Hdr len %d, content len %d\n", aLen, aContentLen);
}

static err_t HttpRecvCallback(void *aArg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    (void)conn;
    (void)aArg;
    if (err == ERR_OK)
    {
        printf("Get data payload len %d\n", p->tot_len);
    }

    return ERR_OK;
}

void demo101Task(void *p)
{
    ip_addr_t          serverAddr;
    httpc_connection_t httpSettings;
    altcp_allocator_t  allocator;

    sDemoTask = *static_cast<TaskHandle_t *>(p);
    WaitForSignal(BUTTON_BIT);

    allocator.alloc = altcp_tcp_alloc;
    allocator.arg   = NULL;

    memset(&httpSettings, 0, sizeof(httpSettings));
    httpSettings.use_proxy       = 0;
    httpSettings.result_fn       = HttpDoneCallback;
    httpSettings.headers_done_fn = HttpHeaderCallback;
    httpSettings.altcp_allocator = &allocator;

    // ifconfig up
    OT_API_CALL(otIp6SetEnabled(otrGetInstance(), true));
    // joiner start
    OT_API_CALL(otJoinerStart(otrGetInstance(), DEMO_PASSPHRASE, NULL, "OTR_VENDOR", "OTR_MODEL", "OTR_VERSION", NULL,
                              HandleJoinerCallback, NULL));

    WaitForSignal(COMMISSION_BIT);

    // thread start
    OT_API_CALL(otThreadSetEnabled(otrGetInstance(), true));
    // wait a while for thread to connect
    vTaskDelay(2000);

    // dns64 www.google.com
    dnsNat64Address("www.google.com", &serverAddr.u_addr.ip6);
    serverAddr.type = IPADDR_TYPE_V6;

    // periodically curl www.google.com
    while (true)
    {
        uint32_t notifyValue;

        httpc_state_t *connection;
        httpc_get_file(&serverAddr, 80, "/", &httpSettings, HttpRecvCallback, NULL, &connection);
        WaitForSignal(HTTP_BIT);
        if (xTaskNotifyWait(BUTTON_BIT, BUTTON_BIT, &notifyValue, 10000) == pdTRUE)
        {
            break;
        }
    }

    vTaskDelete(NULL);
}

extern "C" void GPIOTE_IRQHandler(void)
{
    BaseType_t woken;

    if (nrf_gpiote_event_is_set(NRF_GPIOTE_EVENTS_IN_0))
    {
        nrf_gpio_pin_toggle(LED1_PIN);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_0);
        xTaskNotifyFromISR(sDemoTask, BUTTON_BIT, eSetBits, &woken);
        if (woken)
        {
            portEND_SWITCHING_ISR(woken);
        }
    }
}

void demo101Init(void)
{
    nrf_gpio_cfg_output(LED1_PIN);
    nrf_gpio_pin_write(LED1_PIN, 0);
    nrf_gpio_cfg_input(BUTTON1_PIN, NRF_GPIO_PIN_PULLUP);
    nrf_gpiote_event_enable(0);
    nrf_gpiote_event_configure(0, BUTTON1_PIN, NRF_GPIOTE_POLARITY_HITOLO);
    nrf_gpio_cfg_input(BUTTON2_PIN, NRF_GPIO_PIN_PULLUP);
    nrf_gpiote_event_enable(1);
    nrf_gpiote_event_configure(1, BUTTON2_PIN, NRF_GPIOTE_POLARITY_HITOLO);

    nrf_gpiote_int_enable(NRF_GPIOTE_INT_IN0_MASK);
    NVIC_SetPriority(GPIOTE_IRQn, GPIO_PRIORITY);
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);
    NVIC_EnableIRQ(GPIOTE_IRQn);
    xTaskCreate(demo101Task, "demo", 3000, &sDemoTask, 2, &sDemoTask);
}

void otrUserInit(void)
{
    demo101Init();
}
