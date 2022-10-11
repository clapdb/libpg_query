cmake_minimum_required(VERSION 3.19.0)

# position matters, must define before project
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(libpg_query)

set(CMAKE_C_FLAGS "-g -Wno-unused-function -Wno-unused-value -Wno-unused-variable -fno-strict-aliasing -fwrapv -fPIC")

# use -save-temps to generate .i/.ii code after pre-process
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps=obj")
#set(CMAKE_CXX_FLAGS "-save-temps=obj")

set(CMAKE_C_FLAGS_DEBUG "-O0 -DUSE_ASSERT_CHECKING")
set(CMAKE_C_FLAGS_RELEASE "-O3")

include_directories(SYSTEM ${PROJECT_SOURCE_DIR})
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/vendor)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/src/postgres/include)


if(NOT memory_path)
    set(memory_path ${PROJECT_SOURCE_DIR}/thirdparty/memory)
endif()
# memory
message(STATUS ${memory_path})
include_directories(${memory_path}/)

## set to c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

# fmt
find_package(fmt REQUIRED)

# sanitizer
set(SANITIZE_PARAM "-fsanitize=address -fsanitize=undefined -fno-sanitize=vptr -fno-omit-frame-pointer")  # param from seastar.pc
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZE_PARAM}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZE_PARAM}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZE_PARAM}")
set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} ${SANITIZE_PARAM}")

# build arena
if (NOT TARGET arena)
  add_subdirectory(thirdparty/memory/arena)
endif()

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


# 依赖 stdb/metas/pg_query.fbe 生成的代码
if (NOT FBE_GEN_CODE_DIR)
  message(FATAL_ERROR "no FBE_GEN_CODE_DIR provided, maybe stdb/metas/generated ?")
endif()
message(STATUS "FBE_GEN_CODE_DIR: ${FBE_GEN_CODE_DIR}")
include_directories(SYSTEM ${FBE_GEN_CODE_DIR})

LIST(APPEND SRC
  ${PROJECT_SOURCE_DIR}/vendor/protobuf-c/protobuf-c.c
  ${PROJECT_SOURCE_DIR}/vendor/xxhash/xxhash.c
  ${PROJECT_SOURCE_DIR}/protobuf/pg_query.pb-c.c
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

# test
list(APPEND TEST_FLAGS -I. -I./vendor -g)
list(APPEND TEST_LDFLAGS -pthread)

LIST(APPEND GEN_SRC
   ${FBE_GEN_CODE_DIR}/fbe.cpp
   ${FBE_GEN_CODE_DIR}/fbe_models.cpp
   ${FBE_GEN_CODE_DIR}/pg_query_ptr.cpp
   ${FBE_GEN_CODE_DIR}/pg_query_ptr_models.cpp
   )
add_library(pg_query_gen EXCLUDE_FROM_ALL ${GEN_SRC})

add_executable(pg_query_parse_fbe_test EXCLUDE_FROM_ALL test/parse_fbe_test.cc)
target_compile_options(pg_query_parse_fbe_test PRIVATE ${TEST_FLAGS})
target_link_libraries(pg_query_parse_fbe_test pg_query pg_query_gen uuid arena fmt::fmt ${TEST_LDFLAGS})

add_executable(pg_parse_demo pg_parse_demo.cc)
target_compile_options(pg_parse_demo PRIVATE ${TEST_FLAGS})
target_link_libraries(pg_parse_demo pg_query pg_query_gen uuid arena fmt::fmt ${TEST_LDFLAGS})
