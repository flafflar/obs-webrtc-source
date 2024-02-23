cmake_minimum_required(VERSION 3.16...3.26)

include(FetchContent)
FetchContent_Declare(libwebsockets_project
  GIT_REPOSITORY https://libwebsockets.org/repo/libwebsockets
  GIT_TAG v4.3-stable
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

message(STATUS "Donwloading libwebsockets")
FetchContent_MakeAvailable(libwebsockets_project)

# TODO: Make this cross-platform
target_compile_options(websockets PUBLIC -fPIC)