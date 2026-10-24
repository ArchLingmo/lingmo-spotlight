# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles/lingmo-spotlight_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/lingmo-spotlight_autogen.dir/ParseCache.txt"
  "lingmo-spotlight_autogen"
  )
endif()
