cmake_minimum_required(VERSION 3.19.0)

project(libpg_query)

set(CMAKE_C_FLAGS "-g -Wno-unused-function -Wno-unused-value -Wno-unused-variable -fno-strict-aliasing -fwrapv -fPIC")

set(CMAKE_C_FLAGS_DEBUG "-O0 -DUSE_ASSERT_CHECKING")
set(CMAKE_C_FLAGS_RELEASE "-O3")

include_directories(SYSTEM ${PROJECT_SOURCE_DIR})
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/vendor)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/src/postgres/include)

# fmt
find_package(fmt REQUIRED)

# sanitizer
set(SANITIZE_PARAM "-fsanitize=address -fsanitize=undefined -fno-sanitize=vptr -fno-omit-frame-pointer") # param from seastar.pc
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZE_PARAM}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZE_PARAM}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZE_PARAM}")
set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} ${SANITIZE_PARAM}")

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
)

add_library(pg_query ${SRC})
