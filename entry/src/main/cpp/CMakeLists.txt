cmake_minimum_required(VERSION 3.5)
project(mrp_napi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_RPATH TRUE)

set(OHOS_LIB_ABI "${OHOS_ARCH}")
if(NOT OHOS_LIB_ABI)
  set(OHOS_LIB_ABI "arm64-v8a")
endif()

add_subdirectory(third_party/vmrp-core)
add_subdirectory(third_party/unicorn)

add_library(mrp_napi SHARED
  napi/module_init.cpp
  napi/facade_binding.cpp
  platform/MrpFacade.cpp
  runtime/MrpAudioBridge.cpp
  runtime/MrpRuntime.cpp
  runtime/VmrpHostBridge.cpp
  runtime/XComponentSurfaceRenderer.cpp
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

target_link_libraries(mrp_napi PRIVATE ace_napi.z ace_ndk.z hilog_ndk.z native_window ohaudio avplayer native_media_core vmrp_core unicorn_headers unicorn_prebuilt)
