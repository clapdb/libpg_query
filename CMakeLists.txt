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

# Platform-specific library naming (from Makefile)
if(APPLE)
    set(SOLIB "libpg_query.dylib")
    set(SONAME "libpg_query.${SO_VERSION}.dylib")
    set(SOLIBVER "libpg_query.${SO_VERSION}.${VERSION_PATCH}.dylib")
    set(SOFLAG "-install_name")
else()
    set(SOLIB "libpg_query.so")
    set(SONAME "${SOLIB}.${SO_VERSION}")
    set(SOLIBVER "${SONAME}.${VERSION_PATCH}")
    set(SOFLAG "-soname")
endif()

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

# Create shared library (SOLIB from Makefile)
add_library(pg_query_shared SHARED ${FINAL_SRC_FILES})
target_include_directories(pg_query_shared PUBLIC ${INCLUDE_DIRS})
target_compile_options(pg_query_shared PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${ALL_CFLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${CXXFLAGS}>
)
target_link_options(pg_query_shared PRIVATE -shared)

if(USE_PROTOBUF_CPP)
    target_link_libraries(pg_query_shared ${PROTOBUF_LIBRARIES})
endif()

# Set shared library properties
set_target_properties(pg_query_shared PROPERTIES
    OUTPUT_NAME pg_query
    SOVERSION ${SO_VERSION}
    VERSION ${SO_VERSION}.${VERSION_PATCH}
)

# Test flags (from Makefile lines 46-47)
set(TEST_CFLAGS -g -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/vendor -Wall -std=c99 -D_GNU_SOURCE)
set(TEST_LDFLAGS -pthread)

if(USE_PROTOBUF_CPP)
    list(APPEND TEST_LDFLAGS ${PROTOBUF_LIBRARIES} -lstdc++ -lm)
endif()

if(VALGRIND)
    list(APPEND TEST_CFLAGS -DUSE_VALGRIND)
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

# Tests (from Makefile line 243)
set(TESTS complex concurrency deparse fingerprint fingerprint_opts normalize normalize_utility parse parse_opts parse_protobuf parse_protobuf_opts parse_plpgsql scan split)

# Test source file mappings - note that *_tests.c files are included by the main test files
set(complex_SOURCES test/complex.c)
set(concurrency_SOURCES test/concurrency.c)
set(deparse_SOURCES test/deparse.c)
set(fingerprint_SOURCES test/fingerprint.c)
set(fingerprint_opts_SOURCES test/fingerprint_opts.c)
set(normalize_SOURCES test/normalize.c)
set(normalize_utility_SOURCES test/normalize_utility.c)
set(parse_SOURCES test/parse.c)
set(parse_opts_SOURCES test/parse_opts.c)
set(parse_plpgsql_SOURCES test/parse_plpgsql.c)
set(parse_protobuf_SOURCES test/parse_protobuf.c)
set(parse_protobuf_opts_SOURCES test/parse_protobuf_opts.c)
set(scan_SOURCES test/scan.c)
set(split_SOURCES test/split.c)

foreach(test ${TESTS})
    add_executable(test_${test} ${${test}_SOURCES})
    target_include_directories(test_${test} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
    target_compile_options(test_${test} PRIVATE ${TEST_CFLAGS})

    # Some tests need src/ directory access (from Makefile comments)
    if(test STREQUAL "complex" OR test STREQUAL "fingerprint" OR test STREQUAL "fingerprint_opts")
        target_include_directories(test_${test} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    endif()

    target_link_libraries(test_${test} pg_query ${TEST_LDFLAGS})
    set_target_properties(test_${test} PROPERTIES OUTPUT_NAME ${test})
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

set(TEST_TARGETS)
foreach(test ${TESTS})
    list(APPEND TEST_TARGETS test_${test})
endforeach()

add_custom_target(test_all
    DEPENDS ${TEST_TARGETS}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/complex
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/concurrency
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deparse
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fingerprint
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fingerprint_opts
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/normalize
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/normalize_utility
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/parse
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/parse_opts
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/parse_protobuf
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/parse_protobuf_opts
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/scan
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/split
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/parse_plpgsql
    COMMAND diff -Naur test/plpgsql_samples.expected.json test/plpgsql_samples.actual.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Build target (matches Makefile line 107)
add_custom_target(build DEPENDS pg_query)

# Build shared target (matches Makefile line 109)
add_custom_target(build_shared DEPENDS pg_query_shared)

# All target (matches Makefile line 105) - using build_all to avoid reserved name
add_custom_target(build_all DEPENDS examples test_all build)

# Installation (matches Makefile lines 330-338)
include(GNUInstallDirs)

install(TARGETS pg_query pg_query_shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install with version symlinks for shared library
if(UNIX AND NOT APPLE)
    install(CODE "
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${SOLIBVER}
            \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${SONAME}
        )
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${SOLIBVER}
            \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${SOLIB}
        )
    ")
endif()

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
add_dependencies(pg_query_shared protobuf_files)