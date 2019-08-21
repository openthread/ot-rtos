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

#include "net/utils/time_ntp.h"

#include <FreeRTOS.h>
#include <task.h>

#include <lwip/dns.h>
#include <lwip/netdb.h>

#include <openthread/ip6.h>
#include <openthread/openthread-freertos.h>
#include <openthread/sntp.h>

#include "net/utils/nat64_utils.h"

#define NTP_NOTIFY_VALUE (1 << 11)

struct NtpContext
{
    TaskHandle_t mTaskHandle;
    uint64_t     mTime;
    otError      mErr;
};

void ntpHandle(void *aContext, uint64_t aTime, otError aResult)
{
    NtpContext *ctx = static_cast<NtpContext *>(aContext);
    ctx->mErr       = aResult;
    ctx->mTime      = aTime;

    xTaskNotify(ctx->mTaskHandle, NTP_NOTIFY_VALUE, eSetBits);
}

uint64_t timeNtp()
{
    otInstance *  instance = otrGetInstance();
    otSntpQuery   query;
    otMessageInfo messageInfo;
    NtpContext    ctx;
    uint32_t      notifyValue = 0;
    ip6_addr_t    serverAddr;
    if (dnsNat64Address("time.google.com", &serverAddr) == 0)
    {
        memset(&messageInfo, 0, sizeof(messageInfo));
        messageInfo.mIsHostInterface = false;
        memcpy(&messageInfo.mPeerAddr, serverAddr.addr, sizeof(messageInfo.mPeerAddr));
        messageInfo.mPeerPort = OT_SNTP_DEFAULT_SERVER_PORT;

        query.mMessageInfo = &messageInfo;

        ctx.mTaskHandle = xTaskGetCurrentTaskHandle();
        OT_API_CALL(otSntpClientQuery(instance, &query, ntpHandle, &ctx));

        while ((notifyValue & NTP_NOTIFY_VALUE) == 0)
        {
            xTaskNotifyWait(0, NTP_NOTIFY_VALUE, &notifyValue, portMAX_DELAY);
        }

        if (ctx.mErr != OT_ERROR_NONE)
        {
            ctx.mTime = 0;
        }
    }
    else
    {
        ctx.mTime = 0;
    }

    return ctx.mTime;
}
