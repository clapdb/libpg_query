cmake_minimum_required(VERSION 3.19.0)

# position matters, must define before project
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(libpg_query)

set(CMAKE_C_FLAGS "-g -Wno-unused-function -Wno-unused-value -Wno-unused-variable -fno-strict-aliasing -fwrapv -fPIC")

# use -save-temps to generate .i/.ii code after pre-process
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps=obj")
#set (CMAKE_CXX_FLAGS "-save-temps=obj")

set(CMAKE_C_FLAGS_DEBUG "-O0 -DUSE_ASSERT_CHECKING")
set(CMAKE_C_FLAGS_RELEASE "-O3")

include_directories(SYSTEM ${PROJECT_SOURCE_DIR})
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/vendor)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/src/postgres/include)

## set to c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)


FILE(GLOB_RECURSE SRC
  ${PROJECT_SOURCE_DIR}/src/*.c)

LIST(REMOVE_ITEM SRC
  ${PROJECT_SOURCE_DIR}/src/pg_query_enum_defs.c
  ${PROJECT_SOURCE_DIR}/src/pg_query_fingerprint_defs.c
  ${PROJECT_SOURCE_DIR}/src/pg_query_fingerprint_conds.c
  ${PROJECT_SOURCE_DIR}/src/pg_query_outfuncs_defs.c
  ${PROJECT_SOURCE_DIR}/src/pg_query_outfuncs_conds.c
  ${PROJECT_SOURCE_DIR}/src/pg_query_readfuncs_defs.c
  ${PROJECT_SOURCE_DIR}/src/pg_query_readfuncs_conds.c
  ${PROJECT_SOURCE_DIR}/src/pg_query_json_helper.c
  ${PROJECT_SOURCE_DIR}/src/postgres/guc-file.c
  ${PROJECT_SOURCE_DIR}/src/postgres/scan.c)

LIST(APPEND SRC
  ${PROJECT_SOURCE_DIR}/vendor/protobuf-c/protobuf-c.c
  ${PROJECT_SOURCE_DIR}/vendor/xxhash/xxhash.c
  ${PROJECT_SOURCE_DIR}/protobuf/pg_query.pb-c.c
  ${PROJECT_SOURCE_DIR}/fbe/fbe.cpp
  ${PROJECT_SOURCE_DIR}/fbe/fbe_models.cpp
  ${PROJECT_SOURCE_DIR}/fbe/pg_query_ptr.cpp
  ${PROJECT_SOURCE_DIR}/fbe/pg_query_ptr_models.cpp
  ${PROJECT_SOURCE_DIR}/src/pg_query_outfuncs_fbe_cpp.cc
  ${PROJECT_SOURCE_DIR}/src/pg_query_parse_fbe.cc
  )

## compile flags
list(APPEND CXX_FLAGS_LIST
  -frtti
  -fexceptions
  -fsized-deallocation

  # https://github.com/vadimcn/vscode-lldb/issues/415
  -fstandalone-debug
)

string(JOIN " " BASE_CXX_FLAGS ${CXX_FLAGS_LIST})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BASE_CXX_FLAGS}")
link_libraries(-Wl,--as-needed) # as-needed is required to skip librte*.so

add_library(pg_query ${SRC})

list(APPEND TEST_FLAGS -I. -I./vendor -g)
list(APPEND TEST_LDFLAGS -pthread)

add_executable(pg_query_parse_fbe_test test/parse_fbe_test.cc)
target_compile_options(pg_query_parse_fbe_test PRIVATE ${TEST_FLAGS})
target_link_libraries(pg_query_parse_fbe_test pg_query ${TEST_LDFLAGS})
