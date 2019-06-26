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

# Keep use bash avoid escape issues on different platforms
set(SHELL "bash")

include(ExternalProject)

if (${PLATFORM_NAME} STREQUAL nrf52)
    set(OT_PLATFORM_NAME nrf52840)
    set(OT_BUILD_DIR nrf52840)
elseif(${PLATFORM_NAME} STREQUAL linux)
    set(OT_PLATFORM_NAME posix)
    set(OT_BUILD_DIR x86_64-unknown-linux-gnu)
else()
    set(OT_PLATFORM_NAME ${PLATFORM_NAME})
endif()

message("PLATFORM_NAME is ${PLATFORM_NAME}")
message("OT_PLATFORM_NAME is ${OT_PLATFORM_NAME}")

set(PLATFORM_CONFIG_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/examples/platforms/${OT_PLATFORM_NAME}
)

if (${PLATFORM_NAME} STREQUAL nrf52)
    set(PLATFORM_CONFIG_INCLUDE_DIRECTORIES
        ${PLATFORM_CONFIG_INCLUDE_DIRECTORIES}
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/dependencies
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/drivers/clock
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/libraries/app_error
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/libraries/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/libraries/nrf_cc310/include
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/nrfx
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/nrfx/drivers/include
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/nrfx/hal
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/nrfx/mdk
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/cmsis
    )
endif()

set(OT_CPPFLAGS "-DOT_CLI_UART_LOCK_HDR_FILE=\"uart_lock.h\" -DOPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS=4 -I$<JOIN:$<TARGET_PROPERTY:otr_core,INTERFACE_INCLUDE_DIRECTORIES>, -I> -I$<JOIN:$<TARGET_PROPERTY:otr_core,INTERFACE_INCLUDE_DIRECTORIES>, -I> -D$<JOIN:$<TARGET_PROPERTY:otr_core,INTERFACE_COMPILE_DEFINITIONS>, -D>")
set(OT_SWITCHES "JOINER=1 SNTP_CLIENT=1 DISABLE_BUILTIN_MBEDTLS=1 DISABLE_EXECUTABLE=1")

if (${PLATFORM_NAME} STREQUAL nrf52 AND USB_CDC)
    set(OT_SWITCHES "${OT_SWITCHES} USB=1")
endif()

ExternalProject_Add(ot
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/repo
    CONFIGURE_COMMAND ${SHELL} -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/repo && ./bootstrap" && env CPPFLAGS=${OT_CPPFLAGS} ${SHELL} -c "CPPFLAGS=\${CPPFLAGS//\\\"/\\\\\\\\\\\\\\\"} make -f ${CMAKE_CURRENT_SOURCE_DIR}/repo/examples/Makefile-${OT_PLATFORM_NAME} ${OT_SWITCHES} configure"
    BUILD_COMMAND env CPPFLAGS=${OT_CPPFLAGS} ${SHELL} -c "CPPFLAGS=\${CPPFLAGS//\\\"/\\\\\\\\\\\\\\\"} make -f ${CMAKE_CURRENT_SOURCE_DIR}/repo/examples/Makefile-${OT_PLATFORM_NAME} ${OT_SWITCHES}"
    INSTALL_COMMAND cmake -E echo "Skipping install step."
    BUILD_ALWAYS TRUE
    EXCLUDE_FROM_ALL TRUE
)
ExternalProject_Get_Property(ot BINARY_DIR)
ExternalProject_Add_StepTargets(ot configure)

add_library(openthread_config INTERFACE)
add_dependencies(openthread_config ot-configure)

target_include_directories(openthread_config
    INTERFACE
        ${BINARY_DIR}/build/${OT_BUILD_DIR}/include
        ${BINARY_DIR}/output/include
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/include
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/mbedtls/
        ${PLATFORM_CONFIG_INCLUDE_DIRECTORIES}
)

target_compile_definitions(openthread_config
    INTERFACE
        OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"openthread-core-${OT_PLATFORM_NAME}-config.h\"
        OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS=4
)

if (${PLATFORM_NAME} STREQUAL nrf52)
    target_compile_definitions(openthread_config
        INTERFACE
            NRF52840_XXAA
    )
endif()

add_library(mbedtls_platform_config INTERFACE)

if (${PLATFORM_NAME} STREQUAL nrf52)
    target_include_directories(mbedtls_platform_config
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/libraries/crypto/
    )

    target_compile_definitions(mbedtls_platform_config
        INTERFACE
            MBEDTLS_USER_CONFIG_FILE=\"nrf52840-mbedtls-config.h\"
    )
endif()

if (${PLATFORM_NAME} STREQUAL nrf52)
    add_library(nrf_cc310 STATIC IMPORTED)
    set_property(TARGET nrf_cc310 PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/libraries/nrf_cc310/lib/libnrf_cc310_0.9.10.a)
endif()

add_library(openthread_diag STATIC IMPORTED)
set_property(TARGET openthread_diag PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/output/${OT_BUILD_DIR}/lib/libopenthread-diag.a)

add_library(openthread_platform_utils STATIC IMPORTED)
set_property(TARGET openthread_platform_utils PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/output/${OT_BUILD_DIR}/lib/libopenthread-platform-utils.a)

add_library(openthread_ftd STATIC IMPORTED)
set_property(TARGET openthread_ftd PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/output/${OT_BUILD_DIR}/lib/libopenthread-ftd.a)
set_property(TARGET openthread_ftd PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES mbedtls)

add_library(openthread_cli_ftd STATIC IMPORTED)
set_property(TARGET openthread_cli_ftd PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/output/${OT_BUILD_DIR}/lib/libopenthread-cli-ftd.a)
set_property(TARGET openthread_cli_ftd PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES otr_core)

add_library(ot_platform_${PLATFORM_NAME} STATIC IMPORTED)
set_property(TARGET ot_platform_${PLATFORM_NAME} PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/output/${OT_BUILD_DIR}/lib/libopenthread-${OT_PLATFORM_NAME}.a)
set_property(TARGET ot_platform_${PLATFORM_NAME} PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES openthread_ftd)

if (${PLATFORM_NAME} STREQUAL nrf52)
    add_library(nrf52840_radio STATIC IMPORTED)
    set_property(TARGET nrf52840_radio PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/output/${OT_BUILD_DIR}/lib/libnordicsemi-nrf52840-radio-driver.a)
    add_library(nrf52840_sdk STATIC IMPORTED)
    set_property(TARGET nrf52840_sdk PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/output/${OT_BUILD_DIR}/lib/libnordicsemi-nrf52840-sdk.a)
    set_property(TARGET ot_platform_${PLATFORM_NAME} PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES nrf52840_radio nrf52840_sdk)
endif()

add_library(platform_${PLATFORM_NAME} INTERFACE)

target_include_directories(platform_${PLATFORM_NAME}
    INTERFACE
        ${BINARY_DIR}/output/include
        ${BINARY_DIR}/build/${OT_BUILD_DIR}/include
        ${PLATFORM_CONFIG_INCLUDE_DIRECTORIES}
)

target_link_libraries(platform_${PLATFORM_NAME}
    INTERFACE
        ot_platform_${PLATFORM_NAME}
)

if (${PLATFORM_NAME} STREQUAL nrf52)
    target_link_libraries(platform_${PLATFORM_NAME}
        INTERFACE
            nrf52840_radio
            nrf52840_sdk
    )
endif()

target_compile_definitions(platform_${PLATFORM_NAME}
    INTERFACE
        PLATFORM_${PLATFORM_NAME}
)

if (${PLATFORM_NAME} STREQUAL nrf52)
    target_compile_definitions(platform_${PLATFORM_NAME}
        INTERFACE
            CONFIG_GPIO_AS_PINRESET
            NRF52840_XXAA
    )

    if (USB_CDC)
        target_compile_definitions(platform_${PLATFORM_NAME}
            INTERFACE
                USB_CDC_AS_SERIAL_TRANSPORT=1
        )
    endif()

endif()

add_library(openthread INTERFACE)

target_link_libraries(openthread
    INTERFACE
        mbedcrypto
        openthread_cli_ftd
        openthread_diag
        openthread_ftd
        openthread_platform_utils
        ot_platform_${PLATFORM_NAME}
)

if (${PLATFORM_NAME} STREQUAL nrf52)
    target_link_libraries(platform_${PLATFORM_NAME}
        INTERFACE
            nrf_cc310
    )
endif()

target_include_directories(openthread INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/examples/platforms
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/examples/platforms/${OT_PLATFORM_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/core
)

if (${PLATFORM_NAME} STREQUAL nrf52)
    target_include_directories(openthread INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/third_party/NordicSemiconductor/
    )
endif()

add_dependencies(openthread ot)
add_dependencies(ot mbedcrypto)
