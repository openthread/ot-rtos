set(FREERTOS_PORTABLE_NRF52_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/port_cmsis.c
    ${CMAKE_CURRENT_SOURCE_DIR}/port_cmsis_systick.c
)

add_library(freertos_portable_nrf52 ${FREERTOS_PORTABLE_NRF52_SRCS})

target_include_directories(freertos_portable_nrf52 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/)

target_link_libraries(freertos_portable_nrf52
    PUBLIC
        openthread-${OT_PLATFORM}
        freertos_port_hdrs
)

target_compile_options(freertos_portable_nrf52
    PRIVATE
       -Wall
       -Wextra
       -Wshadow
       -Werror
)
