#
#  Copyright (c) 2020, The OpenThread Authors.
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

add_executable(ot_cli_${OT_PLATFORM}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/io_redirect.c
)

target_include_directories(ot_cli_${OT_PLATFORM}
    PUBLIC
        ${OTR_SRC_ROOT}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(ot_cli_${OT_PLATFORM}
    PRIVATE
        ${OTR_DEFINITIONS}
)

if (${OT_PLATFORM} STREQUAL nrf52 AND OTR_DEMO_101)
    target_link_libraries(ot_cli_${OT_PLATFORM}
    PRIVATE
        otr_demo_101_app
        openthread-cli-ftd
    )
else()
    target_link_libraries(ot_cli_${OT_PLATFORM}
    PRIVATE
        otr_test_app
        openthread-cli-ftd
    )
endif()

if (${OT_PLATFORM} STREQUAL nrf52)
    target_compile_definitions(ot_cli_${OT_PLATFORM}
        PUBLIC
            __HEAP_SIZE=8192
            __STACK_SIZE=8192
    )

    #special link script
    set_target_properties(ot_cli_${OT_PLATFORM} PROPERTIES LINK_DEPENDS ${OTR_SRC_ROOT}/third_party/openthread/repo/examples/platforms/nrf528xx/nrf52840/nrf52840.ld)
    set_target_properties(ot_cli_${OT_PLATFORM} PROPERTIES LINK_FLAGS "-T ${OTR_SRC_ROOT}/third_party/openthread/repo/examples/platforms/nrf528xx/nrf52840/nrf52840.ld -lc -lnosys -lm -lstdc++")

    #build hex file
    add_custom_command(OUTPUT ot_cli_${OT_PLATFORM}.hex
        COMMAND arm-none-eabi-objcopy -O ihex ot_cli_${OT_PLATFORM} ot_cli_${OT_PLATFORM}.hex
        DEPENDS ot_cli_${OT_PLATFORM}
    )
    add_custom_target(ot_cli_nrf52_hex ALL DEPENDS ot_cli_${OT_PLATFORM}.hex)
endif()

