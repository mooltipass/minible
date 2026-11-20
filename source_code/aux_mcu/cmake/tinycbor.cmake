# Managing tinycbor dependency
include(FetchContent)
FetchContent_Declare(
  tinycbor_lib
  GIT_REPOSITORY https://github.com/intel/tinycbor
  GIT_TAG d393c16f3eb30d0c47e6f9d92db62272f0ec4dc7 # TAG V0.6.0
)

FetchContent_MakeAvailable(tinycbor_lib)

set(CBOR_SRC
  ${tinycbor_lib_SOURCE_DIR}/src/cborerrorstrings.c
  ${tinycbor_lib_SOURCE_DIR}/src/cborencoder.c
  ${tinycbor_lib_SOURCE_DIR}/src/cborencoder_close_container_checked.c
  ${tinycbor_lib_SOURCE_DIR}/src/cborencoder_float.c
  ${tinycbor_lib_SOURCE_DIR}/src/cborparser.c
  ${tinycbor_lib_SOURCE_DIR}/src/cborparser_float.c
  ${tinycbor_lib_SOURCE_DIR}/src/cborpretty.c
)
add_library(tinycbor STATIC ${CBOR_SRC})
target_include_directories(tinycbor PUBLIC ${tinycbor_lib_SOURCE_DIR}/src)
