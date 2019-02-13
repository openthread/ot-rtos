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
 *   This file implements lwip net interface with OpenThread.
 */

#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/udp.h>
#include <semphr.h>

#include <openthread/icmp6.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/message.h>
#include <openthread/openthread-freertos.h>
#include <openthread/thread.h>
#include <openthread/platform/radio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"

#include "lwip/dns.h"
#include "lwip/sockets.h"

#include "netif.h"

static const size_t  kMaxIp6Size            = 1500;
static const uint8_t kMulticastPrefixLength = 128;

/**
 * This structure stores information needed to send a IPv6 packet.
 *
 */
struct OutputEvent
{
    OutputEvent *mNext;
    uint16_t     mLength;
    uint8_t      mData[1];
};

static SemaphoreHandle_t sGuardOutput = NULL;
static OutputEvent *     sHeadOutput  = NULL;
static OutputEvent *     sLastOutput  = NULL;
static struct netif      sNetif;

static bool IsLinkLocal(const struct otIp6Address &aAddress)
{
    return aAddress.mFields.m16[0] == htons(0xfe80);
}

static void HandleNetifStatus(struct netif *aNetif)
{
    // TODO process events from lwip
    (void)aNetif;
    otLogInfoPlat("LwIP netif event");
}

static err_t netifOutputIp6(struct netif *aNetif, struct pbuf *aBuffer, const ip6_addr_t *aPeerAddr)
{
    (void)aPeerAddr;

    err_t        err   = ERR_OK;
    OutputEvent *event = NULL;

    otLogInfoPlat("netif output");
    assert(aNetif == &sNetif);
    event = (OutputEvent *)malloc(sizeof(*event) + aBuffer->tot_len);

    event->mLength = aBuffer->tot_len;
    event->mNext   = NULL;
    VerifyOrExit(event != NULL, err = ERR_BUF);
    VerifyOrExit(aBuffer->tot_len == pbuf_copy_partial(aBuffer, event->mData, aBuffer->tot_len, 0), err = ERR_ARG);

    xSemaphoreTake(sGuardOutput, portMAX_DELAY);
    if (sLastOutput == NULL)
    {
        sHeadOutput = event;
        sLastOutput = event;
    }
    else
    {
        sLastOutput->mNext = event;
    }
    xSemaphoreGive(sGuardOutput);

    otrTaskNotifyGive();

exit:
    if (err != ERR_OK)
    {
        if (event != NULL)
        {
            free(event);
        }
    }
    return err;
}

static err_t netifInit(struct netif *aNetif)
{
    aNetif->name[0]    = 'o';
    aNetif->name[1]    = 't';
    aNetif->hwaddr_len = sizeof(otExtAddress);
    memset(aNetif->hwaddr, 0, sizeof(aNetif->hwaddr));
    aNetif->mtu    = OPENTHREAD_CONFIG_IPV6_DEFAULT_MAX_DATAGRAM;
    aNetif->flags  = NETIF_FLAG_BROADCAST;
    aNetif->output = NULL;
#if LWIP_IPV6
    aNetif->output_ip6 = netifOutputIp6;
#endif
    aNetif->num = 0;

    return ERR_OK;
}

struct netif *otrGetNetif(void)
{
    return &sNetif;
}

static void addAddress(const otIp6Address &aAddress)
{
    otError error = OT_ERROR_NONE;
    err_t   err   = ERR_OK;

    LOCK_TCPIP_CORE();
    if (IsLinkLocal(aAddress))
    {
        netif_ip6_addr_set(&sNetif, 0, reinterpret_cast<const ip6_addr_t *>(&aAddress));
        netif_ip6_addr_set_state(&sNetif, 0, IP6_ADDR_PREFERRED);
    }
    else
    {
        int8_t                   index  = -1;
        const otMeshLocalPrefix *prefix = otThreadGetMeshLocalPrefix(otrGetInstance());

        err = netif_add_ip6_address(&sNetif, reinterpret_cast<const ip6_addr_t *>(&aAddress), &index);
        VerifyOrExit(err == ERR_OK && index != -1, error = OT_ERROR_FAILED);
        if (memcmp(&aAddress, prefix, sizeof(prefix->m8)) != 0)
        {
            netif_ip6_addr_set_state(&sNetif, index, IP6_ADDR_PREFERRED);
        }
        else
        {
            netif_ip6_addr_set_state(&sNetif, index, IP6_ADDR_VALID);
        }
    }

exit:
    UNLOCK_TCPIP_CORE();

    if (error != OT_ERROR_NONE)
    {
        otLogInfoPlat("Failed to add address: %d", error);
    }
}

static void delAddress(const otIp6Address &aAddress)
{
    int8_t index;

    LOCK_TCPIP_CORE();
    index = netif_get_ip6_addr_match(&sNetif, reinterpret_cast<const ip6_addr_t *>(&aAddress));
    VerifyOrExit(index != -1);
    netif_ip6_addr_set_state(&sNetif, index, IP6_ADDR_INVALID);

exit:
    UNLOCK_TCPIP_CORE();
}

static void setupDns(void)
{
    ip_addr_t dnsServer;

    inet_pton(AF_INET6, "64:ff9b::808:808", &dnsServer.u_addr.ip6.addr);
    dnsServer.type            = IPADDR_TYPE_V6;
    dnsServer.u_addr.ip6.zone = IP6_NO_ZONE;

    dns_init();
    dns_setserver(0, &dnsServer);
}

static void processStateChange(otChangedFlags aFlags, void *aContext)
{
    otInstance *instance = static_cast<otInstance *>(aContext);

    if (OT_CHANGED_THREAD_NETIF_STATE | aFlags)
    {
        LOCK_TCPIP_CORE();
        if (otLinkIsEnabled(instance))
        {
            otLogInfoPlat("netif up");
            netif_set_up(&sNetif);
        }
        else
        {
            otLogInfoPlat("netif down");
            netif_set_down(&sNetif);
        }
        UNLOCK_TCPIP_CORE();
    }
}

static void processAddress(const otIp6Address *aAddress, uint8_t aPrefixLength, bool aIsAdded, void *aContext)
{
    (void)aContext;

    otLogInfoPlat("address changed");

    if (aPrefixLength != kMulticastPrefixLength)
    {
        if (aIsAdded)
        {
            addAddress(*aAddress);
        }
        else
        {
            delAddress(*aAddress);
        }
    }
}

static void processReceive(otMessage *aMessage, void *aContext)
{
    otError      error      = OT_ERROR_NONE;
    err_t        err        = ERR_OK;
    const size_t kBlockSize = 128;
    uint16_t     length     = otMessageGetLength(aMessage);
    struct pbuf *buffer     = NULL;
    otInstance * aInstance  = static_cast<otInstance *>(aContext);

    assert(sNetif.state == aInstance);

    buffer = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);

    VerifyOrExit(buffer != NULL, error = OT_ERROR_NO_BUFS);

    for (uint16_t i = 0; i < length; i += kBlockSize)
    {
        uint8_t block[kBlockSize];
        int     count = otMessageRead(aMessage, i, block, sizeof(block));

        assert(count > 0);
        err = pbuf_take_at(buffer, block, count, i);
        VerifyOrExit(err == ERR_OK, error = OT_ERROR_FAILED);
    }

    VerifyOrExit(ERR_OK == sNetif.input(buffer, &sNetif), error = OT_ERROR_FAILED);

exit:
    if (error != OT_ERROR_NONE)
    {
        if (buffer != NULL)
        {
            pbuf_free(buffer);
        }

        if (error == OT_ERROR_FAILED)
        {
            otLogWarnPlat("%s failed for lwip error %d", __func__, err);
        }

        otLogWarnPlat("%s failed: %s", __func__, otThreadErrorToString(error));
    }

    otMessageFree(aMessage);
}

static void processTransmit(otInstance *aInstance)
{
    otError    error   = OT_ERROR_NONE;
    otMessage *message = NULL;

    VerifyOrExit(sHeadOutput != NULL);

    message = otIp6NewMessage(aInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = otMessageAppend(message, sHeadOutput->mData, sHeadOutput->mLength));

    error   = otIp6Send(aInstance, message);
    message = NULL;

    {
        OutputEvent *nextEvent = sHeadOutput->mNext;

        free(sHeadOutput);
        sHeadOutput = nextEvent;
    }

    // Notify if more
    if (sHeadOutput != NULL)
    {
        otrTaskNotifyGive();
    }
    else
    {
        sLastOutput = NULL;
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        if (message != NULL)
        {
            otMessageFree(message);
        }

        otLogWarnPlat("Failed to transmit IPv6 packet: %s", otThreadErrorToString(error));
    }
}

void netifInit(void *aContext)
{
    otInstance *instance = static_cast<otInstance *>(aContext);

    memset(&sNetif, 0, sizeof(sNetif));
    // LOCK_TCPIP_CORE();
    netif_add(&sNetif, NULL, NULL, NULL, instance, netifInit, tcpip_input);
    netif_set_link_up(&sNetif);
    netif_set_status_callback(&sNetif, HandleNetifStatus);
    // UNLOCK_TCPIP_CORE();

    sGuardOutput = xSemaphoreCreateMutex();
    xSemaphoreGive(sGuardOutput);
    assert(sGuardOutput != NULL);
    otLogInfoPlat("Initialize netif");

    otIp6SetAddressCallback(instance, processAddress, instance);
    otIp6SetReceiveCallback(instance, processReceive, instance);
    otSetStateChangedCallback(instance, processStateChange, instance);
    otIp6SetReceiveFilterEnabled(instance, true);
    otIcmp6SetEchoMode(instance, OT_ICMP6_ECHO_HANDLER_DISABLED);

    netif_set_default(&sNetif);

    setupDns();
}

void netifProcess(otInstance *aInstance)
{
    if (sGuardOutput != NULL && xSemaphoreTake(sGuardOutput, portMAX_DELAY) == pdTRUE)
    {
        processTransmit(aInstance);
        xSemaphoreGive(sGuardOutput);
    }
}
