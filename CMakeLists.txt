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

project(openthread_freertos)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-Os")
set(CMAKE_C_FLAGS_RELEASE "-Os")

set(FIRST_PARTY_COMPILE_FLAGS
   -Wall
   -Wextra
   -Wshadow
   -Werror
   $<$<COMPILE_LANGUAGE:CXX>:
       -std=gnu++14
       -fno-exceptions
       -fno-rtti
   >
)

add_subdirectory(third_party/freertos)
add_subdirectory(third_party/freertos_portable)
add_subdirectory(third_party/freertos-addons)
add_subdirectory(third_party/openthread)
add_subdirectory(third_party/jansson)
add_subdirectory(third_party/libjwt)
add_subdirectory(third_party/lwip)
add_subdirectory(third_party/mbedtls)

add_subdirectory(examples/apps)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(otr_core_utils
    ${SRC_DIR}/core/utils/entropy_utils.c
)

target_include_directories(otr_core_utils
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(otr_core_utils
    PRIVATE
        openthread
)

target_compile_options(otr_core_utils
    PRIVATE
        ${FIRST_PARTY_COMPILE_FLAGS}
)

add_library(otr_core
    ${SRC_DIR}/core/netif.cpp
    ${SRC_DIR}/core/openthread_freertos.c
    ${SRC_DIR}/core/otr_system.c
    ${SRC_DIR}/core/uart_lock.c
)

target_include_directories(otr_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(otr_core
    PUBLIC
        openthread
        freertos
        mbedtls
        lwip
)


add_library(otr_frameworks
    ${SRC_DIR}/net/utils/nat64_utils.c
    ${SRC_DIR}/net/utils/time_ntp.cpp
)

target_link_libraries(otr_frameworks
    PUBLIC
        otr_core
        jansson
        libjwt
)

target_compile_options(otr_frameworks
    PRIVATE
        ${FIRST_PARTY_COMPILE_FLAGS}
)

add_executable(ot_cli_${PLATFORM_NAME}
    ${SRC_DIR}/apps/cli/main.c
    ${SRC_DIR}/core/io_redirect.c
)

target_link_libraries(ot_cli_${PLATFORM_NAME}
    PUBLIC
        test_app
)

if (${PLATFORM_NAME} STREQUAL nrf52)
    add_executable(ot_demo_101
        ${SRC_DIR}/apps/cli/main.c
        ${SRC_DIR}/core/io_redirect.c
    )

    target_link_libraries(ot_demo_101
        PUBLIC
            demo_101
    )

    target_compile_definitions(ot_demo_101
        PUBLIC
            __HEAP_SIZE=8192
            __STACK_SIZE=8192
    )

    target_compile_definitions(ot_cli_${PLATFORM_NAME}
        PUBLIC
            __HEAP_SIZE=8192
            __STACK_SIZE=8192
    )

    #special link script
    set_target_properties(ot_cli_${PLATFORM_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openthread/repo/examples/platforms/nrf528xx/nrf52840/nrf52840.ld)
    set_target_properties(ot_cli_${PLATFORM_NAME} PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openthread/repo/examples/platforms/nrf528xx/nrf52840/nrf52840.ld -lc -lnosys -lm -lstdc++")
    set_target_properties(ot_demo_101 PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openthread/repo/examples/platforms/nrf528xx/nrf52840/nrf52840.ld)
    set_target_properties(ot_demo_101 PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openthread/repo/examples/platforms/nrf528xx/nrf52840/nrf52840.ld -lc -lnosys -lm -lstdc++")

    #build hex file
    add_custom_command(OUTPUT ot_cli_${PLATFORM_NAME}.hex
        COMMAND arm-none-eabi-objcopy -O ihex ot_cli_${PLATFORM_NAME} ot_cli_${PLATFORM_NAME}.hex
        DEPENDS ot_cli_${PLATFORM_NAME}
    )
    add_custom_target(ot_cli_nrf52_hex ALL DEPENDS ot_cli_${PLATFORM_NAME}.hex)

    add_custom_command(OUTPUT ot_demo_101.hex
        COMMAND arm-none-eabi-objcopy -O ihex ot_demo_101 ot_demo_101.hex
        DEPENDS ot_demo_101
    )
    add_custom_target(ot_demo_101_hex ALL DEPENDS ot_demo_101.hex)
endif()

set(PORT_DIRS 
    ./third_party/freertos-addons/port
    ./third_party/lwip/port
)
