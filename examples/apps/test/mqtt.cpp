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

#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/netdb.h"
#include "lwip/tcpip.h"

#include "jwt.h"

#include <stdlib.h>
#include <string.h>

#include "apps/google_cloud_iot/client_cfg.h"
#include "apps/google_cloud_iot/mqtt_client.hpp"
#include "net/utils/nat64_utils.h"
#include "net/utils/time_ntp.h"

#include <openthread/openthread-freertos.h>

#define MQTT_CLIENT_NOTIFY_VALUE (1 << 9)

namespace ot {
namespace app {

struct ConnectContext
{
    GoogleCloudIotMqttClient *mClient;
    TaskHandle_t              mHandle;
};

static const int           kQos     = 1;
static const unsigned long kTimeout = 10000L;

static const unsigned long kInitialConnectIntervalMillis     = 500L;
static const unsigned long kMaxConnectIntervalMillis         = 6000L;
static const unsigned long kMaxConnectRetryTimeElapsedMillis = 900000L;
static const float         kIntervalMultiplier               = 1.5f;

void GoogleCloudIotMqttClient::mqttPubChanged(void *aArg, err_t aResult)
{
    (void)aArg;

    if (aResult == ERR_OK)
    {
        printf("Publish done\r\n");
    }
    else
    {
        printf("Publish get error %d\r\n", aResult);
    }
}

void GoogleCloudIotMqttClient::mqttConnectChanged(mqtt_client_t *aClient, void *aArg, mqtt_connection_status_t aResult)
{
    (void)aClient;

    ConnectContext *ctx = static_cast<ConnectContext *>(aArg);

    if (aResult == MQTT_CONNECT_ACCEPTED)
    {
        printf("Mqtt Connected\r\n");
    }

    if (aResult != MQTT_CONNECT_DISCONNECTED)
    {
        ctx->mClient->mConnectResult = aResult;
        xTaskNotify(ctx->mHandle, MQTT_CLIENT_NOTIFY_VALUE, eSetBits);
    }
}

static void GetIatExp(char *aIat, char *aExt, int time_size)
{
    time_t now_seconds = timeNtp();

    snprintf(aIat, (size_t)time_size, "%zu", (size_t)now_seconds);
    snprintf(aExt, (size_t)time_size, "%zu", (size_t)(now_seconds + 3600));
}

static const char *CreateJwt(const char *aPrivKey, const char *aProjectId, jwt_alg_t aAlgorithm)
{
    char        iatTime[sizeof(uint64_t) * 3 + 2];
    char        expTime[sizeof(uint64_t) * 3 + 2];
    const char *key    = aPrivKey;
    size_t      keyLen = strlen(aPrivKey) + 1;
    jwt_t *     jwt    = NULL;
    int         ret    = 0;
    char *      out    = NULL;

    // Get JWT parts
    GetIatExp(iatTime, expTime, sizeof(iatTime));

    jwt_new(&jwt);

    // Write JWT
    ret = jwt_add_grant(jwt, "iat", iatTime);
    if (ret)
    {
        printf("Error setting issue timestamp: %d\r\n", ret);
    }
    ret = jwt_add_grant(jwt, "exp", expTime);
    if (ret)
    {
        printf("Error setting expiration: %d\r\n", ret);
    }
    ret = jwt_add_grant(jwt, "aud", aProjectId);
    if (ret)
    {
        printf("Error adding audience: %d\r\n", ret);
    }
    ret = jwt_set_alg(jwt, aAlgorithm, reinterpret_cast<const uint8_t *>(key), keyLen);
    if (ret)
    {
        printf("Error during set alg: %d\r\n", ret);
    }
    out = jwt_encode_str(jwt);
    if (!out)
    {
        printf("Error during token creation:\r\n");
    }

    jwt_free(jwt);
    return out;
}

GoogleCloudIotMqttClient::GoogleCloudIotMqttClient(const GoogleCloudIotClientCfg &aConfig)
    : mConfig(aConfig)
    , mMqttClient(NULL)
{
}

int GoogleCloudIotMqttClient::Connect()
{
    int       ret         = 0;
    uint32_t  notifyValue = 0;
    ip_addr_t serverAddr;

    mMqttClient = mqtt_client_new();
    memset(&mClientInfo, 0, sizeof(mClientInfo));
    mClientInfo.client_id   = mConfig.mClientId;
    mClientInfo.keep_alive  = 60;
    mClientInfo.client_user = NULL;
    mClientInfo.client_pass = CreateJwt(mConfig.mPrivKey, mConfig.mProjectId, mConfig.mAlgorithm);
    mClientInfo.tls_config  = altcp_tls_create_config_client_2wayauth(
        NULL, 0, reinterpret_cast<const uint8_t *>(mConfig.mPrivKey), strlen(mConfig.mPrivKey) + 1, NULL, 0,
        reinterpret_cast<const uint8_t *>(mConfig.mRootCertificate), strlen(mConfig.mRootCertificate) + 1);

    if (dnsNat64Address(mConfig.mAddress, &serverAddr.u_addr.ip6) == 0)
    {
        struct ConnectContext ctx;

        ctx.mClient     = this;
        ctx.mHandle     = xTaskGetCurrentTaskHandle();
        serverAddr.type = IPADDR_TYPE_V6;

        LOCK_TCPIP_CORE();
        mqtt_client_connect(mMqttClient, &serverAddr, 8883, &GoogleCloudIotMqttClient::mqttConnectChanged, &ctx,
                            &mClientInfo);
        UNLOCK_TCPIP_CORE();

        while ((notifyValue & MQTT_CLIENT_NOTIFY_VALUE) == 0)
        {
            xTaskNotifyWait(0, MQTT_CLIENT_NOTIFY_VALUE, &notifyValue, portMAX_DELAY);
        }
        ret = (mConnectResult == 0) ? 0 : -1;
    }
    else
    {
        ret = -1;
    }

    return ret;
}

int GoogleCloudIotMqttClient::Publish(const char *aTopic, const uint8_t *aMsg, size_t aMsgLength)
{
    mqtt_publish(mMqttClient, aTopic, aMsg, aMsgLength, kQos, 0, &GoogleCloudIotMqttClient::mqttPubChanged, NULL);

    return 0;
}

GoogleCloudIotMqttClient::~GoogleCloudIotMqttClient()
{
    if (mMqttClient)
    {
        mqtt_client_free(mMqttClient);
    }
    if (mClientInfo.tls_config)
    {
        altcp_tls_free_config(mClientInfo.tls_config);
    }
    if (mClientInfo.client_pass)
    {
        free(const_cast<char *>(mClientInfo.client_pass));
    }
}

} // namespace app
} // namespace ot

void mqttTask(void *p)
{
    static uint8_t message[8] = "Hello";
    (void)p;
    int count = 10;

    ot::app::GoogleCloudIotClientCfg cfg;

    printf("Mqtt task\r\n");

    cfg.mAddress         = CLOUDIOT_SERVER_ADDRESS;
    cfg.mClientId        = CLOUDIOT_CLIENT_ID;
    cfg.mDeviceId        = CLOUDIOT_DEVICE_ID;
    cfg.mRegistryId      = CLOUDIOT_REGISTRY_ID;
    cfg.mProjectId       = CLOUDIOT_PROJECT_ID;
    cfg.mRegion          = CLOUDIOT_REGION;
    cfg.mRootCertificate = CLOUDIOT_CERT;
    cfg.mPrivKey         = CLOUDIOT_PRIV_KEY;
    cfg.mAlgorithm       = JWT_ALG_RS256;

    ot::app::GoogleCloudIotMqttClient client(cfg);

    client.Connect();

    printf("Connect done \n");

    client.Publish("/devices/test-device/events", message, sizeof(message));

    while (count--)
    {
        printf("tick\n");
        vTaskDelay(10000);
    }

    gTestTask = NULL;
    vTaskDelete(NULL);
}
