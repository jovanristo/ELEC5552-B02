# file-format: 1.0.0
cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

include(cmake/target.cmake)
include(cmake/${TOOLCHAIN_FILE})

project(ELEC5552-NewMCU)
add_executable(ELEC5552-NewMCU)

include(cmake/files.cmake)
include(cmake/flags.cmake)
include(cmake/components.cmake)

# Add our own firmware source files
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  user_modifiable/app/app.c
  user_modifiable/inputs/inputs.c
  user_modifiable/outputs/outputs.c
  user_modifiable/timers/timers.c
  user_modifiable/fan/fan.c
  user_modifiable/protection/protection.c
  user_modifiable/usb/usb.c
  user_modifiable/storage/storage.c
  user_modifiable/logger/logger.c
  user_modifiable/control/control.c
  user_modifiable/display/display.c
  user_modifiable/user_limits/user_limits.c
)

# Add our own firmware header locations
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/app
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/inputs
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/outputs
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/timers
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/fan
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/protection
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/usb
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/storage
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/logger
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/control
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/display
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/user_limits
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional defined symbols here
)

target_compile_options(ELEC5552-NewMCU PRIVATE
    -Os
)

target_link_options(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional linker flags here
)

target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional linker search directories here
)

add_subdirectory(middleware/lvgl)

target_compile_options(lvgl PRIVATE
  -mcpu=cortex-m33
  -mfpu=fpv5-sp-d16
  -mfloat-abi=hard
  -mthumb
  -ffunction-sections
  -fdata-sections
)

target_link_libraries(${CMAKE_PROJECT_NAME}
  lvgl
)