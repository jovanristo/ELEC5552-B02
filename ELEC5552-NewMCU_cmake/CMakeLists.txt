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
)

# Add our own firmware header locations
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/app
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/inputs
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/outputs
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/timers
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/fan
  ${CMAKE_CURRENT_SOURCE_DIR}/user_modifiable/protection
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional defined symbols here
)

target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional compiler flags here
)

target_link_options(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional linker flags here
)

target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional linker search directories here
)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional linker libraries here
)