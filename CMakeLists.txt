cmake_minimum_required(VERSION 3.28)

# Include the ARM toolchan
include(gcc-arm-none-eabi.cmake)

project(freertos LANGUAGES ASM C CXX VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)



set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F446RETX_FLASH.ld)

file(GLOB_RECURSE FREERTOS_SOURCES 
    ${CMAKE_SOURCE_DIR}/Sources/*.c
    ${CMAKE_SOURCE_DIR}/Sources/*.cpp
    ${CMAKE_SOURCE_DIR}/Sources/*.cc
    ${CMAKE_SOURCE_DIR}/Core/Startup/*.s
    ${CMAKE_SOURCE_DIR}/FreeRTOS/*.c
    ${CMAKE_SOURCE_DIR}/tflm/examples/hello_world/models/*.cc

)

# file(GLOB_RECURSE TFLM_SOURCES
#     ${CMAKE_SOURCE_DIR}/tflm/tensorflow/*.c
#     ${CMAKE_SOURCE_DIR}/tflm/third_party/*.cc
# )

# message(STATUS ${FREERTOS_SOURCES})


# Create the main executable target
add_executable(${PROJECT_NAME} ${FREERTOS_SOURCES})


# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/Core/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Core/CMSIS/Device
    ${CMAKE_SOURCE_DIR}/Includes
    ${CMAKE_SOURCE_DIR}/FreeRTOS/include
    ${CMAKE_SOURCE_DIR}/FreeRTOS/portable/GCC/ARM_CM4F
    ${CMAKE_SOURCE_DIR}/tflm
    ${CMAKE_SOURCE_DIR}/tflm/third_party/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/tflm/third_party/gemmlowp
    ${CMAKE_SOURCE_DIR}/tflm/tensorflow/lite
    ${CMAKE_SOURCE_DIR}/tflm/examples/hello_world
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# Link TensorFlow Lite Micro library
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib/libtensorflow-microlite.a)

# Compiler definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC STM32F446xx TF_LITE_STATIC_MEMORY)

# Set compiler options (e.g., enable debugging)
# target_compile_options(${PROJECT_NAME} PUBLIC -g)

# Set linker options
target_link_options(${PROJECT_NAME} PUBLIC 
    -T${LINKER_SCRIPT}
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--print-memory-usage
    -Wl,--gc-sections
    -lc 
    -lrdimon
    -lstdc++
    -lm
)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
)

add_custom_target(flash 
    COMMAND st-flash write ${PROJECT_NAME}.bin 0x08000000
    COMMAND st-flash reset
)