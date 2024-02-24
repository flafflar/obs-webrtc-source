cmake_minimum_required(VERSION 3.16...3.26)

# This download takes too long, so it is better to display the progress
set(FETCHCONTENT_QUIET OFF)

include(FetchContent)
FetchContent_Declare(libdatachannel_project
  GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel
  GIT_TAG v0.20.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

# Examples do not build correctly for some reason, so I turned them off
set(NO_EXAMPLES ON)

message(STATUS "Donwloading libdatachannel")
FetchContent_MakeAvailable(libdatachannel_project)
