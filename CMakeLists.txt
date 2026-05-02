cmake_minimum_required(VERSION 3.28)

# Read version from package.json
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/package.json" PACKAGE_JSON_CONTENT)
string(JSON PROJECT_VERSION_FROM_PACKAGE_JSON GET "${PACKAGE_JSON_CONTENT}" "version")

project(uuid_v8 VERSION ${PROJECT_VERSION_FROM_PACKAGE_JSON} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Get current year for copyright notice
string(TIMESTAMP CURRENT_YEAR "%Y")

# --- Git Versioning Logic ---
find_package(Git QUIET)
set(UUID_V8_FULL_VERSION "${PROJECT_VERSION}")
set(UUID_V8_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(UUID_V8_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(UUID_V8_VERSION_PATCH ${PROJECT_VERSION_PATCH})

if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Only use GIT_VERSION for the full version string if it is not "dirty".
    # This prevents the release commit from being tagged as "dirty" in the header.
    if(GIT_VERSION AND NOT GIT_VERSION MATCHES "dirty")
        set(UUID_V8_FULL_VERSION "${GIT_VERSION}")
    endif()

    # Git Hook Automation
    set(HOOK_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/pre-commit.sh")
    set(HOOK_DEST "${CMAKE_CURRENT_SOURCE_DIR}/.git/hooks/pre-commit")
    
    if(EXISTS "${HOOK_SOURCE}")
        if(UNIX)
            execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${HOOK_SOURCE}" "${HOOK_DEST}")
            file(CHMOD "${HOOK_SOURCE}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        else()
            execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${HOOK_SOURCE}" "${HOOK_DEST}")
        endif()
        message(STATUS "Git pre-commit hook installed/updated.")
    endif()

endif()

# Generate the header file directly in the source include directory
set(UUID_V8_SOURCE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/uuid_v8/uuid_v8.hpp")
set(UUID_V8_BINARY_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/uuid_v8/uuid_v8.hpp")

# 1. Generate the header file in the source directory (the "master" for distribution)
configure_file(uuid_v8.hpp.in "${UUID_V8_SOURCE_HEADER}" @ONLY)

# 2. Ensure the build include directory exists
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/uuid_v8")

# 3. Create a symbolic link in the build directory; fallback to copy on non-Unix systems
if(UNIX)
    file(CREATE_LINK "${UUID_V8_SOURCE_HEADER}" "${UUID_V8_BINARY_HEADER}" SYMBOLIC)
else()
    file(CREATE_LINK "${UUID_V8_SOURCE_HEADER}" "${UUID_V8_BINARY_HEADER}" COPY)
endif()

# Header-only library target
add_library(uuid_v8 INTERFACE)
add_library(uuid_v8::uuid_v8 ALIAS uuid_v8)

target_include_directories(uuid_v8 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Dependencies
find_package(Intl REQUIRED)
target_link_libraries(uuid_v8 INTERFACE ${Intl_LIBRARIES})

# Tests
option(UUID_V8_BUILD_TESTS "Build uuid_v8 unit tests" ON)

if(UUID_V8_BUILD_TESTS)
    enable_testing()
    add_executable(test_uuid_v8 tests/test_uuid_v8.cpp)
    target_link_libraries(test_uuid_v8 PRIVATE uuid_v8)

    # Pass version information directly from CMake to the tests via macros
    target_compile_definitions(test_uuid_v8 PRIVATE
        CMAKE_EXPECTED_VERSION_MAJOR=${UUID_V8_VERSION_MAJOR}
        CMAKE_EXPECTED_VERSION_MINOR=${UUID_V8_VERSION_MINOR}
        CMAKE_EXPECTED_VERSION_PATCH=${UUID_V8_VERSION_PATCH}
        CMAKE_EXPECTED_VERSION="${UUID_V8_FULL_VERSION}"
    )
    
    add_test(NAME uuid_v8_unit_tests COMMAND test_uuid_v8)
endif()

install(TARGETS uuid_v8 EXPORT uuid_v8Targets)
install(FILES "${UUID_V8_SOURCE_HEADER}" DESTINATION include/uuid_v8)
install(EXPORT uuid_v8Targets
    FILE uuid_v8Config.cmake
    DESTINATION lib/cmake/uuid_v8)