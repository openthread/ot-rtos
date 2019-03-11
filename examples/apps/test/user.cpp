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

#include "user.h"

#include <stdio.h>
#include <string.h>

#include <openthread/cli.h>
#include <openthread/error.h>
#include <openthread/openthread-freertos.h>

#include "google_cloud_iot/client_cfg.h"
#include "google_cloud_iot/mqtt_client.hpp"

TaskHandle_t                     gTestTask = NULL;
static ot::app::GoogleCloudIotClientCfg sCloudIotCfg;

static otError parseLong(char *argv, long *aValue)
{
    char *endptr;
    *aValue = strtol(argv, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

static void ProcessTest(int argc, char *argv[])
{
    if (argc < 1)
    {
        return;
    }

    if (gTestTask != NULL)
    {
        otCliAppendResult(OT_ERROR_BUSY);
    }
    else if (!strcmp(argv[0], "http"))
    {
        xTaskCreate(httpTask, "http", 2048, otrGetInstance(), 2, &gTestTask);
    }
    else if (!strcmp(argv[0], "mqtt"))
    {
        sCloudIotCfg.mAddress         = CLOUDIOT_SERVER_ADDRESS;
        sCloudIotCfg.mRootCertificate = CLOUDIOT_CERT;
        sCloudIotCfg.mAlgorithm       = JWT_ALG_RS256;
        sCloudIotCfg.mClientId   = CLOUDIOT_CLIENT_ID;
        sCloudIotCfg.mDeviceId   = CLOUDIOT_DEVICE_ID;
        sCloudIotCfg.mRegistryId = CLOUDIOT_REGISTRY_ID;
        sCloudIotCfg.mProjectId  = CLOUDIOT_PROJECT_ID;
        sCloudIotCfg.mRegion     = CLOUDIOT_REGION;
        sCloudIotCfg.mPrivKey    = CLOUDIOT_PRIV_KEY;

        xTaskCreate(mqttTask, "mqtt", 3000, &sCloudIotCfg, 2, &gTestTask);
    }
}

static void ProcessEchoServer(int argc, char *argv[])
{
    long port;

    if (argc != 1)
    {
        otCliAppendResult(OT_ERROR_PARSE);
        return;
    }

    if (parseLong(argv[0], &port) != OT_ERROR_NONE)
    {
        otCliAppendResult(OT_ERROR_PARSE);
        return;
    }

    if (!startTcpEchoServer(otrGetInstance(), (uint16_t)port))
    {
        otCliAppendResult(OT_ERROR_BUSY);
        return;
    }
}

static void ProcessConnect(int argc, char *argv[])
{
    long port;

    if (argc != 2)
    {
        otCliAppendResult(OT_ERROR_PARSE);
        return;
    }

    if (parseLong(argv[1], &port) != OT_ERROR_NONE)
    {
        otCliAppendResult(OT_ERROR_PARSE);
        return;
    }

    if (!startTcpConnect(otrGetInstance(), argv[0], (uint16_t)port))
    {
        otCliAppendResult(OT_ERROR_BUSY);
        return;
    }
}

static void ProcessDisconnect(int argc, char *argv[])
{
    UNUSED_VARIABLE(argc);
    UNUSED_VARIABLE(argv);

    if (!startTcpDisconnect())
    {
        otCliAppendResult(OT_ERROR_BUSY);
        return;
    }
}

bool startTcpSend(otInstance *aInstance, uint32_t count, uint32_t size);

static void ProcessSend(int argc, char *argv[])
{
    long count;
    long size;

    if (argc != 2)
    {
        otCliAppendResult(OT_ERROR_PARSE);
        return;
    }

    if (parseLong(argv[1], &count) != OT_ERROR_NONE)
    {
        otCliAppendResult(OT_ERROR_PARSE);
        return;
    }

    if (parseLong(argv[0], &size) != OT_ERROR_NONE)
    {
        otCliAppendResult(OT_ERROR_PARSE);
        return;
    }

    if (!startTcpSend(otrGetInstance(), (uint16_t)count, (uint16_t)size))
    {
        otCliAppendResult(OT_ERROR_BUSY);
        return;
    }
}

static const struct otCliCommand sCommands[] = {{"test", ProcessTest},
                                                {"tcp_echo_server", ProcessEchoServer},
                                                {"tcp_connect", ProcessConnect},
                                                {"tcp_disconnect", ProcessDisconnect},
                                                {"tcp_send", ProcessSend}};

void otrUserInit(void)
{
    otCliSetUserCommands(sCommands, sizeof(sCommands) / sizeof(sCommands[0]));
}
