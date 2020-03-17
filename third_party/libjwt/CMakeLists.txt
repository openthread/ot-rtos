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

project (libjwt)

write_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h "")

set(LIBJWT_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/libjwt/base64.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/libjwt/jwt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/jwt-mbedtls.c
)

add_library(libjwt STATIC ${LIBJWT_SRC_FILES})

target_compile_options(libjwt PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Werror
    -Wno-unused-parameter
    -Wno-pointer-sign
    -Wno-unused-variable
    -Wno-type-limits
    -Wno-unused-function)

target_include_directories(libjwt
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/libjwt
)

target_link_libraries(libjwt
    PRIVATE
        jansson
        mbedtls
        otr_entropy
)
