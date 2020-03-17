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

#ifndef OTBR_RTOS_MQTT_CLIENT_HPP_
#define OTBR_RTOS_MQTT_CLIENT_HPP_

#include "FreeRTOS.h"
#include "jwt.h"
#include "semphr.h"
#include "lwip/apps/mqtt.h"

namespace ot {
namespace app {

struct GoogleCloudIotClientCfg
{
    const char *mAddress;
    const char *mClientId;
    const char *mDeviceId;
    const char *mRegistryId;
    const char *mProjectId;
    const char *mRegion;
    const char *mRootCertificate;
    const char *mPrivKey;

    jwt_alg_t mAlgorithm;
};

class GoogleCloudIotMqttClient
{
public:
    typedef void (*MqttTopicDataCallback)(const char *aTopic, const char *aMsg, uint16_t aMsgLength);

    GoogleCloudIotMqttClient(const GoogleCloudIotClientCfg &aConfig);

    int Connect(void);

    int Publish(const char *aTopic, const char *aMsg, size_t aMsgLength);

    int Subscribe(const char *aTopic, MqttTopicDataCallback aCb);

    ~GoogleCloudIotMqttClient(void);

    static const size_t   kTopicNameMaxLength = 50;
    static const size_t   kTopicDataMaxLength = 201;
    static const uint16_t kMqttPort           = 8883;

private:
    static void MqttPubSubChanged(void *aArg, err_t aResult);

    static void MqttConnectChanged(mqtt_client_t *aClient, void *aArg, mqtt_connection_status_t aStatus);

    static void MqttDataCallback(void *aArg, const uint8_t *aData, uint16_t aLength, uint8_t aFlags);
    void        mqttDataCallback(const uint8_t *aData, uint16_t aLength, uint8_t aFlags);

    static void MqttPublishCallback(void *aArg, const char *aTopic, uint32_t aTotalLength);
    void        mqttPublishCallback(const char *aTopic, uint32_t aTotalLength);

    GoogleCloudIotClientCfg mConfig;

    struct mqtt_connect_client_info_t mClientInfo;
    mqtt_client_t *                   mMqttClient;
    mqtt_connection_status_t          mConnectResult;
    int                               mPubSubResult;

    MqttTopicDataCallback mSubCb;

    // Currently we only support short message with small buffers
    char     mSubTopicNameBuf[kTopicNameMaxLength];
    char     mSubDataBuf[kTopicDataMaxLength];
    uint16_t mDataOffset;
};

} // namespace app
} // namespace ot

#endif
