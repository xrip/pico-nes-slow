# pico-infonesPlus
cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)
# We also need PICO EXTRAS
#include(pico_extras_import.cmake)

pico_sdk_init()


#add_compile_options(-Wall)

#add_subdirectory(drivers/fatfs)
#add_subdirectory(drivers/sdcard)
#add_subdirectory(drivers/ps2kbd)
add_subdirectory(drivers/vga)


project(sandbox C CXX ASM)
add_executable(sandbox
        agnes.c
main.cpp
)

target_compile_definitions(sandbox PRIVATE 
# VGA
VGA_GPIO_FIRST=6 
VGA_GPIO_OUTNUM=6 
VGA_GPIO_SYNC=12
VGA_VSYNC=13
# SDCARD
SDCARD_PIN_SPI0_CS=5
SDCARD_PIN_SPI0_SCK=2
SDCARD_PIN_SPI0_MOSI=3
SDCARD_PIN_SPI0_MISO=4
# PS2KBD
USE_PS2_KBD    
PS2KBD_GPIO_FIRST=0
)

# Pull in our pico_stdlib which aggregates commonly used features
  target_link_libraries(sandbox PRIVATE
  #tinyusb_host
  #tinyusb_board

vga
#sdcard
#fatfs
#ps2kbd

#        pico_stdlib pico_stdio pico_bootrom pico_multicore pico_stdio  pico_multicore
#        hardware_clocks hardware_pio hardware_vreg hardware_pio
#        hardware_sync hardware_pll hardware_spi hardware_irq hardware_dma


    pico_multicore
    pico_stdlib

    hardware_timer
    hardware_vreg
    hardware_dma
    hardware_pio
    hardware_pwm

)
#pico_set_binary_type(sandbox blocked_ram)
pico_set_binary_type(sandbox copy_to_ram)
# enable for on pico debugging
pico_enable_stdio_usb(sandbox 0)
pico_enable_stdio_uart(sandbox 1)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(sandbox)
add_custom_command(TARGET sandbox POST_BUILD COMMAND picotool load sandbox.uf2 && picotool reboot)