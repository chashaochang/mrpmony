if(NOT OHOS_LIB_ABI)
  set(OHOS_LIB_ABI "arm64-v8a")
endif()

add_library(unicorn_headers INTERFACE)
target_include_directories(unicorn_headers INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

add_library(unicorn_prebuilt SHARED IMPORTED GLOBAL)
set_target_properties(unicorn_prebuilt PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/../../../libs/${OHOS_LIB_ABI}/libunicorn.so.2"
)
