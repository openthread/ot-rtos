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

set(FREERTOS_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/croutine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/portable/MemMang/heap_3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/stream_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/timers.c)

add_library(freertos ${FREERTOS_SRCS})

target_include_directories(freertos
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(freertos
    PUBLIC
        freertos_portable
)

target_compile_options(freertos
    PRIVATE
       -Wall
       -Wextra
       -Wshadow
       -Wno-type-limits
       -Werror)

add_library(freertos_port_hdrs INTERFACE)

target_include_directories(freertos_port_hdrs
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/repo/FreeRTOS/Source/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

