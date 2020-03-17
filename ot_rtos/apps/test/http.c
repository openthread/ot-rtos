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

#include <FreeRTOS.h>
#include <lwip/sockets.h>
#include <task.h>

#include <openthread/ip6.h>

#include "test_tasks.h"
#include "ot_rtos/core/openthread_freertos.h"

void httpTask(void *p)
{
    struct sockaddr_in6 saddr;
    struct sockaddr_in6 daddr;
    static char         req[]    = "GET / HTTP/1.1\r\nHost: 106.15.231.211:25680\r\n\r\n";
    otInstance *        instance = (otInstance *)p;
    char                res[512];
    int                 fd   = socket(AF_INET6, SOCK_STREAM, 0);
    int                 rval = 0;
    otMessageInfo       messageInfo;

    memset(&daddr, 0, sizeof(daddr));
    daddr.sin6_family = AF_INET6;
    daddr.sin6_port   = htons(25680);
    inet_pton(AF_INET6, "64:ff9b::808:808", &daddr.sin6_addr);
    memset(&messageInfo, 0, sizeof(messageInfo));
    memcpy(&messageInfo.mPeerAddr, &daddr.sin6_addr, sizeof(messageInfo.mPeerAddr));

    OT_API_CALL(otIp6SelectSourceAddress(instance, &messageInfo));
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_port   = htons(25680);
    memcpy(&saddr.sin6_addr, &messageInfo.mSockAddr, sizeof(saddr.sin6_addr));
    if (fd >= 0)
    {
        printf("http client start\r\n");
    }
    else
    {
        printf("http client not start %d\r\n", fd);
        goto exit;
    }
    if (0 == bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)))
    {
        printf("http bind ok\r\n");
    }
    else
    {
        printf("http not bind\r\n");
    }
    rval = connect(fd, (struct sockaddr *)&daddr, sizeof(daddr));
    if (rval == 0)
    {
        printf("http connect ok\r\n");
    }
    else
    {
        printf("http connect failed %d\r\n", rval);
        goto exit;
    }
    rval = send(fd, req, sizeof(req), 0);
    if (rval > 0)
    {
        printf("http send ok\r\n");
    }
    else
    {
        printf("http send failed\r\n");
        goto exit;
    }
    rval      = recv(fd, res, sizeof(res), 0);
    res[rval] = '\0';
    printf("res:%s\r\n", res);

exit:
    printf("http end\r\n");
    if (fd >= 0)
    {
        close(fd);
    }

    gTestTask = NULL;
    vTaskDelete(NULL);
}
