add_library(vmrp_core STATIC
  bridge.c
  fileLib.c
  memory.c
  network.c
  rbtree.c
  utils.c
  vmrp.c
  mythroad/string.c
  mythroad/md5.c
)

target_include_directories(vmrp_core PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/header
  ${CMAKE_CURRENT_LIST_DIR}/mrc
)

target_compile_definitions(vmrp_core PUBLIC VMRP NETWORK_SUPPORT)

target_compile_options(vmrp_core PRIVATE
  -Wno-int-to-pointer-cast
  -Wno-pointer-to-int-cast
  -Wno-incompatible-pointer-types-discards-qualifiers
  -Wno-unused-function
  -Wno-unused-parameter
)

target_link_libraries(vmrp_core PUBLIC z unicorn_headers unicorn_prebuilt)
