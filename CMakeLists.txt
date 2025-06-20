cmake_minimum_required(VERSION 3.13...3.27)

# initialize the SDK based on PICO_SDK_PATH
include(pico_sdk_import.cmake)

project(pico_dual_core_demo CXX C ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()


add_executable(pico_dual_core_demo
    pico_dual_core_demo.c
)

target_compile_definitions(pico_dual_core_demo PRIVATE
    PICO_DEFAULT_UART_BAUD_RATE=115200
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(pico_dual_core_demo
    pico_stdlib
    pico_multicore
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(pico_dual_core_demo)