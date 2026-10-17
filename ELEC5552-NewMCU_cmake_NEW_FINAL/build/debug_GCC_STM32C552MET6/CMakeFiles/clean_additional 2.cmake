# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "debug_GCC_STM32C552MET6")
  file(REMOVE_RECURSE
  "ELEC5552-NewMCU.elf"
  "ELEC5552-NewMCU.map"
  )
endif()
