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

option(OTR_DEMO_101 "Enable nrf52 demo 101" OFF)

add_library(otr_test_app
    ${CMAKE_CURRENT_SOURCE_DIR}/test/http.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test/mqtt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/tcp_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_tasks.cpp
)

target_include_directories(otr_test_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test/
)

target_compile_definitions(otr_test_app
    PRIVATE
        ${OTR_DEFINITIONS}
)

target_link_libraries(otr_test_app
    PUBLIC
        otr_core
        otr_net_utils
        libjwt
)

add_custom_target(
    client_cfg
    COMMAND
        ${CMAKE_CURRENT_SOURCE_DIR}/script/generate_cloudiot_cfg.sh
        ${CLOUDIOT_DEVICE} ${CLOUDIOT_REGISTRY} ${CLOUDIOT_PROJECT}
        ${CLOUDIOT_REGION} ${CLOUDIOT_PRIV_KEY}
)

add_dependencies(otr_test_app client_cfg)

if (${OT_PLATFORM} STREQUAL nrf52 AND OTR_DEMO_101)
    add_library(otr_demo_101_app
        ${CMAKE_CURRENT_SOURCE_DIR}/demo_101/demo_101.cpp
    )

    target_include_directories(otr_demo_101_app
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/demo_101/
    )

    target_compile_definitions(otr_demo_101_app
        PRIVATE
            ${OTR_DEFINITIONS}
    )

    target_link_libraries(demo_101
        PUBLIC
            otr_core
            otr_net_utils
            libjwt
    )
endif()
