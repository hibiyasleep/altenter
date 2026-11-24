cmake_minimum_required(VERSION 3.20)
project(altenter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(version SHARED
  src/dllmain.cpp
  src/config.cpp
  src/window_hook.cpp
  src/borderless.cpp
  src/log.cpp
  src/proxy.cpp
)

set_target_properties(version PROPERTIES
  OUTPUT_NAME "version"
  PREFIX ""
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_include_directories(version PRIVATE src)
target_compile_definitions(version PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(version PRIVATE user32)
