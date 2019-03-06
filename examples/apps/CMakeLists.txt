#
#  Copyright (c) 2019, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

cmake_minimum_required (VERSION 3.7)

add_library(test_app
    ${CMAKE_CURRENT_SOURCE_DIR}/test/http.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test/mqtt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/tcp_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test/user.cpp
)

target_include_directories(test_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(test_app
    PUBLIC
        otr_frameworks
)

if (DEFINED CLOUDIOT_DEVICE AND DEFINED CLOUDIOT_REGISTRY AND DEFINED CLOUDIOT_PROJECT
        AND DEFINED CLOUDIOT_REGION AND DEFINED CLOUDIOT_PRIV_KEY)
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_SOURCE_DIR}/include/google_cloud_iot/client_cfg.h
        COMMAND
            ${CMAKE_CURRENT_SOURCE_DIR}/script/generate_cloudiot_cfg.sh
            ${CLOUDIOT_DEVICE} ${CLOUDIOT_REGISTRY} ${CLOUDIOT_PROJECT}
            ${CLOUDIOT_REGION} ${CLOUDIOT_PRIV_KEY}
    )

    add_custom_target(client_cfg ALL DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/google_cloud_iot/client_cfg.h)

    add_dependencies(test_app client_cfg)
endif()

if (${PLATFORM_NAME} STREQUAL nrf52)
    add_library(demo_101
        ${CMAKE_CURRENT_SOURCE_DIR}/demo_101/demo_101.cpp
    )

    target_link_libraries(demo_101
        PUBLIC
            otr_frameworks
    )
endif()
