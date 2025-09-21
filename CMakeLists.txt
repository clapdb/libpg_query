cmake_minimum_required(VERSION 3.10)
project(pg_query VERSION 6.1.0 LANGUAGES C CXX)

# Version variables (from Makefile)
set(PG_VERSION "17.4")
set(PG_VERSION_MAJOR 17)
set(PG_VERSION_NUM 170004)
set(PG_BRANCH "REL_17_STABLE")
set(PROTOC_VERSION "25.1")

set(VERSION_MAJOR 6)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

# Calculate SO_VERSION like in Makefile
math(EXPR SO_VERSION_PART1 "${PG_VERSION_MAJOR} * 100 + ${VERSION_MAJOR}")
if(SO_VERSION_PART1 LESS 1000)
    set(SO_VERSION_FORMATTED "${SO_VERSION_PART1}")
else()
    string(SUBSTRING "${SO_VERSION_PART1}" 0 2 SO_MAJOR_PART)
    string(SUBSTRING "${SO_VERSION_PART1}" 2 2 SO_MINOR_PART)
    set(SO_VERSION_FORMATTED "${SO_MAJOR_PART}${SO_MINOR_PART}")
endif()
set(SO_VERSION "${SO_VERSION_FORMATTED}.${VERSION_MINOR}")


# Options (from Makefile)
option(DEBUG "Enable debug build" OFF)
option(VALGRIND "Enable valgrind support" OFF)
option(USE_PROTOBUF_CPP "Use experimental Protobuf C++ library" OFF)

# Source files (from Makefile line 35)
file(GLOB SRC_C_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/postgres/*.c"
)
list(APPEND SRC_C_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/protobuf-c/protobuf-c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/xxhash/xxhash.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb-c.c"
)

# C++ source files (from Makefile line 87)
set(CXX_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pg_query_outfuncs_protobuf_cpp.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb.cc"
)

# Base include directories (from Makefile line 38)
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/postgres/include
)

# Windows-specific includes (from Makefile lines 40-42)
if(WIN32)
    list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/postgres/include/port/win32)
endif()

# Base compiler flags (from Makefile line 38)
set(BASE_CFLAGS
    -Wall
    -Wno-unused-function
    -Wno-unused-value
    -Wno-unused-variable
    -fno-strict-aliasing
    -fwrapv
    -fPIC
)

# Optimization level (from Makefile lines 49-56)
if(DEBUG)
    set(OPT_LEVEL -O0)
elseif(VALGRIND)
    set(OPT_LEVEL -O0)
else()
    set(OPT_LEVEL -O3)
endif()

# Debug flags (from Makefile lines 58-61)
set(DEBUG_FLAGS)
if(DEBUG)
    list(APPEND DEBUG_FLAGS -DUSE_ASSERT_CHECKING)
endif()

# Valgrind flags (from Makefile lines 62-65)
set(VALGRIND_FLAGS)
if(VALGRIND)
    list(APPEND VALGRIND_FLAGS -DUSE_VALGRIND)
endif()

# Combine all C flags
set(ALL_CFLAGS ${BASE_CFLAGS} ${OPT_LEVEL} -g ${DEBUG_FLAGS} ${VALGRIND_FLAGS})

# Determine final source files based on USE_PROTOBUF_CPP
set(FINAL_SRC_FILES ${SRC_C_FILES})

if(USE_PROTOBUF_CPP)
    # Remove files when using C++ protobuf (from Makefile line 98)
    list(REMOVE_ITEM FINAL_SRC_FILES
        "${CMAKE_SOURCE_DIR}/src/pg_query_outfuncs_json.c"
        "${CMAKE_SOURCE_DIR}/src/pg_query_outfuncs_protobuf.c"
    )
    list(APPEND FINAL_SRC_FILES ${CXX_SRC_FILES})

    # Find protobuf C++ library
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PROTOBUF REQUIRED protobuf)

    # C++ flags (from Makefile line 89)
    set(CXXFLAGS
        -std=c++17
        -Wall
        -Wno-unused-function
        -Wno-zero-length-array
        -Wno-c99-extensions
        -fwrapv
        -fPIC
        -DHAVE_PTHREAD
        ${PROTOBUF_CFLAGS}
        ${OPT_LEVEL}
        -g
    )
endif()

# Create static library (ARLIB from Makefile)
add_library(pg_query STATIC ${FINAL_SRC_FILES})
target_include_directories(pg_query PUBLIC ${INCLUDE_DIRS})
target_compile_options(pg_query PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${ALL_CFLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${CXXFLAGS}>
)

if(USE_PROTOBUF_CPP)
    target_link_libraries(pg_query ${PROTOBUF_LIBRARIES})
endif()



# Examples (from Makefile line 216)
set(EXAMPLES simple scan normalize simple_error normalize_error simple_plpgsql)

foreach(example ${EXAMPLES})
    add_executable(example_${example} examples/${example}.c)
    target_include_directories(example_${example} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
    target_compile_options(example_${example} PRIVATE ${TEST_CFLAGS})
    target_link_libraries(example_${example} pg_query ${TEST_LDFLAGS})
    set_target_properties(example_${example} PROPERTIES OUTPUT_NAME ${example})
endforeach()


# Custom targets matching Makefile behavior
set(EXAMPLE_TARGETS)
foreach(example ${EXAMPLES})
    list(APPEND EXAMPLE_TARGETS example_${example})
endforeach()

add_custom_target(examples
    DEPENDS ${EXAMPLE_TARGETS}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/simple
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/scan
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/normalize
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/simple_error
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/normalize_error
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/simple_plpgsql
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)


# Build target (matches Makefile line 107)
add_custom_target(build DEPENDS pg_query)


# All target (matches Makefile line 105) - using build_all to avoid reserved name
add_custom_target(build_all DEPENDS examples build)

# Installation (matches Makefile lines 330-338)
include(GNUInstallDirs)

install(TARGETS pg_query
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES pg_query.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES protobuf/pg_query.proto
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pg_query
)

# Protobuf generation rules (matches Makefile lines 199-214)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb-c.c ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb-c.h
    COMMAND protoc --c_out=. protobuf/pg_query.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.proto
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating C protobuf files"
)

if(USE_PROTOBUF_CPP)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb.h
        COMMAND protoc --cpp_out=. protobuf/pg_query.proto
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.proto
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating C++ protobuf files"
    )
endif()

# Dependencies for protobuf-dependent files
add_custom_target(protobuf_files DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb-c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb-c.h
    $<$<BOOL:${USE_PROTOBUF_CPP}>:${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb.cc>
    $<$<BOOL:${USE_PROTOBUF_CPP}>:${CMAKE_CURRENT_SOURCE_DIR}/protobuf/pg_query.pb.h>
)

add_dependencies(pg_query protobuf_files)