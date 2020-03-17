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
#include <task.h>

#include <lwip/sockets.h>

#include <openthread/ip6.h>
#include <openthread/thread.h>

#include "ot_rtos/core/openthread_freertos.h"

#define MAX_SEND_SIZE 1024

struct ServerParams
{
    otInstance *mInstance;
    uint16_t    mPort;
};

struct ConnectParams
{
    otInstance *mInstance;
    char        mPeerAddr[50];
    uint16_t    mPort;
};

struct SendParams
{
    otInstance *mInstance;
    uint32_t    mCount;
    uint32_t    mSize;
};

static int                  sClientSocket  = -1;
static struct ServerParams  sServerParams  = {};
static struct ConnectParams sConnectParams = {};
static struct SendParams    sSendParams    = {};

TaskHandle_t sServerTask = NULL;
TaskHandle_t sClientTask = NULL;

static void echoServerTask(void *p)
{
    struct ServerParams *params = (struct ServerParams *)p;
    struct sockaddr_in6  saddr;
    char                 res[MAX_SEND_SIZE];

    int fd   = socket(AF_INET6, SOCK_STREAM, 0);
    int afd  = -1;
    int rval = 0;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_port   = htons(params->mPort);

    if (fd < 0)
    {
        printf("tcp_echo_server: Failed to create socket\r\n");
        goto exit;
    }

    rval = bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (rval != 0)
    {
        printf("tcp_echo_server: Cannot bind socket\r\n");
        goto exit;
    }

    rval = listen(fd, 5);
    if (rval != 0)
    {
        printf("tcp_echo_server: Cannot listen on socket\r\n");
        goto exit;
    }
    else
    {
        printf("tcp_echo_server: Listening on port %u\r\n", ntohs(saddr.sin6_port));
    }

    afd = accept(fd, NULL, NULL);

    if (afd >= 0)
    {
        printf("tcp_echo_server: Client connected\r\n");
    }
    else
    {
        printf("tcp_echo_server: Client connection error\r\n");
        goto exit;
    }

    do
    {
        rval = recv(afd, res, sizeof(res) - 1, 0);
        if (rval <= 0)
        {
            if (rval == 0)
            {
                printf("tcp_echo_server: Client closed connection\r\n");
            }
            else
            {
                printf("tcp_echo_server: Connection was reset by client\r\n");
            }
            goto exit;
        }

        res[rval] = '\0';
        printf("tcp_echo_server: Received %dB: %s\r\n", rval, res);

        rval = send(afd, res, rval, 0);
        if (rval <= 0)
        {
            printf("tcp_echo_server: Failed to send data\r\n");
            goto exit;
        }
    } while (1);

exit:
    if (fd >= 0)
    {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    if (afd >= 0)
    {
        shutdown(afd, SHUT_RDWR);
        close(afd);
    }

    printf("tcp_echo_server: Finished\r\n");

    sServerTask = NULL;
    vTaskDelete(NULL);
}

void connectTask(void *p)
{
    struct ConnectParams *connect_params = (struct ConnectParams *)p;

    struct sockaddr_in6 daddr;

    int rval = 0;

    sClientSocket = socket(AF_INET6, SOCK_STREAM, 0);

    memset(&daddr, 0, sizeof(daddr));
    daddr.sin6_family = AF_INET6;
    daddr.sin6_port   = htons(connect_params->mPort);
    inet_pton(AF_INET6, connect_params->mPeerAddr, &daddr.sin6_addr);

    if (sClientSocket < 0)
    {
        printf("tcp_client: Failed to create socket\r\n");
        goto exit;
    }

    rval = connect(sClientSocket, (struct sockaddr *)&daddr, sizeof(daddr));
    if (rval != 0)
    {
        printf("tcp_client: Cannot connect to server\r\n");
        goto exit;
    }
    else
    {
        printf("tcp_client: Connected\r\n");
        sClientTask = NULL;
        vTaskDelete(NULL);
        return;
    }

exit:
    if (sClientSocket >= 0)
    {
        shutdown(sClientSocket, SHUT_RDWR);
        close(sClientSocket);
        sClientSocket = -1;
    }

    sClientTask = NULL;
    vTaskDelete(NULL);
}

void disconnectTask(void *p)
{
    UNUSED_VARIABLE(p);

    if (sClientSocket >= 0)
    {
        shutdown(sClientSocket, SHUT_RDWR);
        close(sClientSocket);
        sClientSocket = -1;
    }

    printf("tcp_client: Disconnected\r\n");

    sClientTask = NULL;
    vTaskDelete(NULL);
}

void sendTask(void *p)
{
    struct SendParams *send_params = (struct SendParams *)p;

    int               rval  = 0;
    static const char req[] = {[0 ...(MAX_SEND_SIZE - 1)] = 'A'};
    char              res[MAX_SEND_SIZE + 1];

    TickType_t start;
    TickType_t stop;
    uint32_t   msec;
    uint32_t   sent  = 0;
    uint32_t   recvd = 0;
    uint32_t   throughput;

    TickType_t lat_send_time;
    TickType_t lat_recv_time;
    uint64_t   lat_sum = 0;
    uint32_t   lat_min = UINT32_MAX;
    uint32_t   lat_max = 0;
    uint32_t   lat;

    start = xTaskGetTickCount();

    for (uint32_t i = 0; i < send_params->mCount; i++)
    {
        printf("tcp_client: Sending data\r\n");

        sent  = 0;
        recvd = 0;

        lat_send_time = xTaskGetTickCount();

        while (sent != send_params->mSize)
        {
            rval = send(sClientSocket, req + sent, send_params->mSize - sent, 0);

            if (rval <= 0)
            {
                printf("tcp_client: Failed to send data\r\n");
                goto exit;
            }

            sent += rval;
        }

        while (recvd != send_params->mSize)
        {
            rval = recv(sClientSocket, res + recvd, send_params->mSize - recvd, 0);

            if (rval <= 0)
            {
                if (rval == 0)
                {
                    printf("tcp_client: Server closed connection\r\n");
                }
                else
                {
                    printf("tcp_client: Connection was reset by server\r\n");
                }

                shutdown(sClientSocket, SHUT_RDWR);
                close(sClientSocket);
                sClientSocket = -1;
                goto exit;
            }

            recvd += rval;
        }

        lat_recv_time = xTaskGetTickCount();

        lat = (lat_recv_time - lat_send_time) * portTICK_PERIOD_MS / 2;
        lat_sum += lat;

        if (lat > lat_max)
        {
            lat_max = lat;
        }

        if (lat < lat_min)
        {
            lat_min = lat;
        }

        res[rval] = '\0';
        printf("tcp_client: Received %dB: %s\r\n", rval, res);
    }

    stop = xTaskGetTickCount();
    msec = (stop - start) * portTICK_PERIOD_MS;

    throughput = (100 * 8 * send_params->mSize * send_params->mCount) / msec;

    printf("tcp_client: Data transmitted : %" PRIu32 " B\r\n", send_params->mSize * send_params->mCount);
    printf("tcp_client: Time             : %" PRIu32 " ms\r\n", msec);
    printf("tcp_client: Throughput       : %" PRIu32 ".%" PRIu32 " Kb/s\r\n", throughput / 100, throughput % 100);
    printf("tcp_client: Latency          : Avg: %" PRIu32 " ms Min: %" PRIu32 " ms, Max: %" PRIu32 " ms\r\n",
           (uint32_t)lat_sum / send_params->mCount, lat_min, lat_max);

exit:
    printf("tcp_client: Send finished\r\n");

    sClientTask = NULL;
    vTaskDelete(NULL);
}

bool startTcpEchoServer(otInstance *aInstance, uint16_t aPort)
{
    if (sServerTask == NULL)
    {
        sServerParams.mInstance = aInstance;
        sServerParams.mPort     = aPort;
        UNUSED_VARIABLE(xTaskCreate(echoServerTask, "echo", MAX_SEND_SIZE + 1024, &sServerParams, 2, &sServerTask));

        return true;
    }

    return false;
}

bool startTcpConnect(otInstance *aInstance, char *aPeer, uint16_t aPort)
{
    if ((sClientTask == NULL) && (sClientSocket < 0))
    {
        sConnectParams.mInstance = aInstance;
        sConnectParams.mPort     = aPort;
        strncpy(sConnectParams.mPeerAddr, aPeer, sizeof(sConnectParams.mPeerAddr));
        UNUSED_VARIABLE(xTaskCreate(connectTask, "conn", 2048, &sConnectParams, 2, &sClientTask));

        return true;
    }

    return false;
}

bool startTcpDisconnect(void)
{
    if ((sClientTask == NULL) && (sClientSocket >= 0))
    {
        UNUSED_VARIABLE(xTaskCreate(disconnectTask, "disc", 2048, NULL, 2, &sClientTask));

        return true;
    }

    return false;
}

bool startTcpSend(otInstance *aInstance, uint32_t count, uint32_t size)
{
    if ((sClientTask == NULL) && (sClientSocket >= 0))
    {
        sSendParams.mInstance = aInstance;
        sSendParams.mCount    = count;
        sSendParams.mSize     = size;
        UNUSED_VARIABLE(xTaskCreate(sendTask, "send", MAX_SEND_SIZE + 1024, &sSendParams, 2, &sClientTask));

        return true;
    }

    return false;
}
