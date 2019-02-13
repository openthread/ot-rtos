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

project (jansson)

set(JANSSON_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/dump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/error.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/hashtable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/hashtable_seed.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/load.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/memory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/pack_unpack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/strbuffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/strconv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/utf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/src/value.c
)

add_library(jansson ${JANSSON_SRC_FILES})

target_compile_options(jansson
    PRIVATE
        -Wall
        -Wextra
        -Wshadow
        -Werror
        -Wno-unused-parameter
        -Wno-unused-function
)

target_compile_definitions(jansson
    PUBLIC
        HAVE_CONFIG_H
)

target_include_directories(jansson
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)
