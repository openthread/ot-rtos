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
#include <string.h>

#include <openthread/ip6.h>
#include <openthread/joiner.h>
#include <openthread/openthread-freertos.h>
#include <openthread/thread.h>

#include <lwip/sockets.h>

#include <nrfx/hal/nrf_gpio.h>

#include "common/code_utils.hpp"
#include "utils/thin_function.h"

enum CommandType
{
    OFF = 0x00,
    ON = 0x01,
    BRIGHTNESS = 0x02,
    MULTICAST = 0x48
};

#define LED_BRIGHTNESS_MIN 0
#define LED_BRIGHTNESS_MAX 100
#define LED_TURN_ON 0
#define LED_TURN_OFF 1
#define LED_NUMBER 4

const size_t MAC_ADDRESS_SIZE = 6;
static const uint32_t LED_PIN[4] = {13, 14, 15, 16};

static TaskHandle_t sAoghTask;

const char            kTestNetworkName[]  = "Openthread-AoGH";
const uint16_t        kTestNetworkPanId   = 0xbeef;
const uint8_t         kTestNetworkChannel = 14;
const otExtendedPanId kTestNetworkXPanId  = {{0xde, 0xad, 0xff, 0xbe, 0xef, 0xff, 0xca, 0xfe}};
const otMasterKey     kTestNetworkKey     = {
    {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}};

class AoghTask
{
public:
    AoghTask(void) {}

    void Initialize(void)
    {
        ConnectToThread();
        InitializeLED();
    }

    void LaunchServer(void)
    {
        AoghServer();
    }

private:
    void ConnectToThread(void)
    {
        OT_API_CALL(
            otThreadSetNetworkName(otrGetInstance(), kTestNetworkName),
            otThreadSetExtendedPanId(otrGetInstance(), &kTestNetworkXPanId),
            otLinkSetPanId(otrGetInstance(), kTestNetworkPanId),
            otLinkSetChannel(otrGetInstance(), kTestNetworkChannel),
            otThreadSetMasterKey(otrGetInstance(), &kTestNetworkKey),
            otIp6SetEnabled(otrGetInstance(), true),
            otThreadSetEnabled(otrGetInstance(), true)
        );
        // simply wait for a small while
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    void InitializeLED(void)
    {
        mBrightness = 0;

        for (int i = 0; i < LED_NUMBER; ++i)
        {
            nrf_gpio_cfg_output(LED_PIN[i]);
            nrf_gpio_pin_write(LED_PIN[i], LED_TURN_OFF);
        }
    }

    int AoghServer(void)
    {
        int          sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        int          ret    = 0;
        sockaddr_in6 bindAddr;
        sockaddr     peerAddr;
        socklen_t    addrLen;

        addrLen = sizeof(peerAddr);
        bindAddr.sin6_family = AF_INET6;
        bindAddr.sin6_addr   = IN6ADDR_ANY_INIT;
        bindAddr.sin6_port   = htons(kAoghPort);

        SuccessOrExit(ret = bind(sockfd, reinterpret_cast<sockaddr *>(&bindAddr), sizeof(bindAddr)));

        while (true)
        {
            socklen_t size;

            VerifyOrExit((size = recvfrom(sockfd, reinterpret_cast<char *>(mBuf), sizeof(mBuf), 0, &peerAddr,
                                          &addrLen)) >= 0);
            UdpHandler(sockfd, mBuf, peerAddr, size);
            taskYIELD();
        }
    exit:
        printf("ERROR: failed to bind socket. sockfd = %d ret = %d\n", sockfd, ret);
        return ret;
    }

    void UdpHandler(uint8_t aSockFd, const uint8_t *aData, const sockaddr &aPeerAddr, uint32_t aSize)
    {
        uint8_t command = *aData;

        switch (command)
        {
            // respond six bytes as a mock mac address to multicast scan
            case MULTICAST:
                uint8_t macAddress[MAC_ADDRESS_SIZE];

                EuiToMacAddress(macAddress);
                sendto(aSockFd, &macAddress, sizeof(macAddress), 0, &aPeerAddr, sizeof(aPeerAddr));
                return;

            // turn off all LEDs
            case OFF:
                ControlLED(0);
                break;

            // turn on all LEDs
            case ON:
                ControlLED(100);
                break;

            case BRIGHTNESS:
                ControlLED(*(aData + 1));
                break;

            default:
                printf("ERROR: unrecognized packet\n");
        }

        sendto(aSockFd, &mBrightness, sizeof(mBrightness), 0, &aPeerAddr, sizeof(aPeerAddr));
    }

    void EuiToMacAddress(uint8_t *aMacAddress)
    {
        otExtAddress extAddress;

        OT_API_CALL(otLinkGetFactoryAssignedIeeeEui64(otrGetInstance(), &extAddress));
        // drop the middle two bytes and flip the sixth bit
        memcpy(aMacAddress, extAddress.m8, 3);
        memcpy(aMacAddress + 3, extAddress.m8 + 5, 3);
        aMacAddress[0] ^= 0x10u;
    }

    uint8_t ControlLED(uint8_t aBrightness)
    {
        if (aBrightness < LED_BRIGHTNESS_MIN)
            aBrightness = LED_BRIGHTNESS_MIN;
        else if (aBrightness > LED_BRIGHTNESS_MAX)
            aBrightness = LED_BRIGHTNESS_MAX;

        printf("Set brightness to %d\n", aBrightness);
        mBrightness = aBrightness;
        // Every time increase (BRIGHTNESS_MAX / LED_NUM) percent of brightness, light up one LED
        for (int i = 0; i < LED_NUMBER; ++i)
            SwitchLED(i, i * LED_BRIGHTNESS_MAX / LED_NUMBER < mBrightness);

        return aBrightness;
    }

    bool SwitchLED(int aId, bool aState)
    {
        printf("Set LED%d [%s]\n", aId + 1, aState ? "ON" : "OFF");
        nrf_gpio_pin_write(LED_PIN[aId], aState ? LED_TURN_ON : LED_TURN_OFF);
    }

    uint8_t mBuf[1500], mBrightness;
    static const uint16_t kAoghPort = 30000;

};

void aoghTask(void *p)
{
    (void) p;
    AoghTask t;

    t.Initialize();
    t.LaunchServer();
    while (true)
    {
        vTaskDelay(1000);
    }
}

void otrUserInit(void)
{
    xTaskCreate(aoghTask, "aogh", 3000, &sAoghTask, 2, &sAoghTask);
}
