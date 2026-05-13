cmake_minimum_required(VERSION 3.5)
project(mrp_napi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_RPATH TRUE)

set(VMRP_CORE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../vmrp-core")
set(UNICORN_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../unicorn-src-proxy")
set(OHOS_LIB_ABI "${OHOS_ARCH}")
if(NOT OHOS_LIB_ABI)
  set(OHOS_LIB_ABI "arm64-v8a")
endif()

add_library(unicorn SHARED IMPORTED)
set_target_properties(unicorn PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/../libs/${OHOS_LIB_ABI}/libunicorn.so.2"
)

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
  ${VMRP_CORE_DIR}/vmrp.c
  ${VMRP_CORE_DIR}/bridge.c
  ${VMRP_CORE_DIR}/memory.c
  ${VMRP_CORE_DIR}/fileLib.c
  ${VMRP_CORE_DIR}/network.c
  ${VMRP_CORE_DIR}/utils.c
  ${VMRP_CORE_DIR}/rbtree.c
  ${VMRP_CORE_DIR}/mythroad/string.c
  ${VMRP_CORE_DIR}/mythroad/md5.c
)

target_include_directories(mrp_napi PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime
  ${CMAKE_CURRENT_SOURCE_DIR}/render
  ${CMAKE_CURRENT_SOURCE_DIR}/input
  ${CMAKE_CURRENT_SOURCE_DIR}/napi
  ${VMRP_CORE_DIR}
  ${VMRP_CORE_DIR}/header
  ${VMRP_CORE_DIR}/mrc
  ${UNICORN_SRC_DIR}/include
)

target_compile_definitions(mrp_napi PRIVATE VMRP NETWORK_SUPPORT)

target_compile_options(mrp_napi PRIVATE
  -Wno-int-to-pointer-cast
  -Wno-pointer-to-int-cast
  -Wno-incompatible-pointer-types-discards-qualifiers
  -Wno-unused-function
  -Wno-unused-parameter
)

target_link_libraries(mrp_napi PRIVATE ace_napi.z ace_ndk.z hilog_ndk.z native_window ohaudio z unicorn)
