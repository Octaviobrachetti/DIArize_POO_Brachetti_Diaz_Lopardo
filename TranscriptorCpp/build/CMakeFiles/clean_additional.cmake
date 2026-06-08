# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\TranscriptorCpp_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\TranscriptorCpp_autogen.dir\\ParseCache.txt"
  "TranscriptorCpp_autogen"
  )
endif()
