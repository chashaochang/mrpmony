if(NOT OHOS_LIB_ABI)
  set(OHOS_LIB_ABI "arm64-v8a")
endif()

add_library(unicorn_headers INTERFACE)
target_include_directories(unicorn_headers INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

set(UNICORN_SHARED_LIB_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../../../libs/${OHOS_LIB_ABI}/libunicorn.so.2"
  CACHE FILEPATH
  "Path to the Unicorn shared library used by mrpmony. You can replace this with a locally rebuilt OHOS libunicorn.so.2."
)

if(NOT EXISTS "${UNICORN_SHARED_LIB_PATH}")
  message(FATAL_ERROR
    "Unicorn shared library not found: ${UNICORN_SHARED_LIB_PATH}\n"
    "Either place libunicorn.so.2 under entry/src/main/libs/${OHOS_LIB_ABI}/,\n"
    "or pass -DUNICORN_SHARED_LIB_PATH=/absolute/path/to/libunicorn.so.2 when configuring CMake.\n"
    "See entry/src/main/cpp/third_party/unicorn/README.md for OHOS rebuild steps."
  )
endif()

add_library(unicorn_prebuilt SHARED IMPORTED GLOBAL)
set_target_properties(unicorn_prebuilt PROPERTIES
  IMPORTED_LOCATION "${UNICORN_SHARED_LIB_PATH}"
)
