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

cmake_minimum_required(VERSION 3.7)

set(ENABLE_TESTING OFF CACHE BOOL "Disable mbedtls test")
set(ENABLE_PROGRAMS OFF CACHE BOOL "Disable mbetls program")

#Required to set add interface level link library for mbedcrypto
SET(CMAKE_POLICY_DEFAULT_CMP0022 NEW)
if (POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()
add_subdirectory(repo)

#unforunately mbedtls used include_directories which is not visisble outside
target_include_directories(mbedcrypto
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/include/
        ${CMAKE_CURRENT_SOURCE_DIR}
        $<TARGET_PROPERTY:openthread_config,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:mbedtls_platform_config,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(mbedcrypto
    PUBLIC
        MBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"
        ENABLE_ECDHE_RSA=1
        $<TARGET_PROPERTY:openthread_config,INTERFACE_COMPILE_DEFINITIONS>
        $<TARGET_PROPERTY:mbedtls_platform_config,INTERFACE_COMPILE_DEFINITIONS>
)

target_link_libraries(mbedcrypto
    INTERFACE
        platform_${PLATFORM_NAME}
)

add_dependencies(mbedcrypto openthread_config)
