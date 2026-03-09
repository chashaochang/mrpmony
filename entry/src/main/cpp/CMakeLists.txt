cmake_minimum_required(VERSION 3.5)
project(mrp_napi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mrp_napi SHARED
  napi/module_init.cpp
  napi/facade_binding.cpp
  platform/MrpFacade.cpp
  runtime/MrpRuntime.cpp
  render/FrameBuffer.cpp
  input/InputDispatcher.cpp
)

target_include_directories(mrp_napi PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime
  ${CMAKE_CURRENT_SOURCE_DIR}/render
  ${CMAKE_CURRENT_SOURCE_DIR}/input
  ${CMAKE_CURRENT_SOURCE_DIR}/napi
)
