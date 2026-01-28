cmake_minimum_required(VERSION 3.16)
project(MeshViewer VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(OpenMP REQUIRED)

# G+Smo (optional, for multipatch geometry support)
option(WITH_GISMO "Enable G+Smo multipatch support" ON)

if(WITH_GISMO)
    # Build hint paths from environment variables
    set(GISMO_HINTS "")
    if(DEFINED ENV{GISMO_ROOT})
        list(APPEND GISMO_HINTS "$ENV{GISMO_ROOT}")
    endif()
    if(DEFINED ENV{GISMO_DIR})
        list(APPEND GISMO_HINTS "$ENV{GISMO_DIR}")
    endif()
    # Common sibling directory
    list(APPEND GISMO_HINTS "${CMAKE_SOURCE_DIR}/../gismo")

    # Allow user to specify path via cmake -DGISMO_ROOT=...
    set(GISMO_ROOT "" CACHE PATH "Path to G+Smo installation or source")
    if(GISMO_ROOT)
        list(INSERT GISMO_HINTS 0 "${GISMO_ROOT}")
    endif()

    # Find G+Smo library
    find_library(GISMO_LIBRARY
        NAMES gismo
        HINTS ${GISMO_HINTS}
        PATH_SUFFIXES lib build/lib
        PATHS
            /usr/local
            /usr
            /opt/gismo
    )

    # Find G+Smo headers (gismo.h is in src/ for source builds)
    find_path(GISMO_INCLUDE_DIR
        NAMES gismo.h
        HINTS ${GISMO_HINTS}
        PATH_SUFFIXES src include include/gismo
        PATHS
            /usr/local
            /usr
            /opt/gismo
    )

    # Find gsEigen (in external/ for source builds)
    find_path(GISMO_EXTERNAL_DIR
        NAMES gsEigen/Core Eigen/Core
        HINTS ${GISMO_HINTS}
        PATH_SUFFIXES external include
        PATHS
            /usr/local
            /usr
            /opt/gismo
    )

    # Find gismo_config.h (in build dir for source builds, or include for installed)
    find_path(GISMO_CONFIG_DIR
        NAMES gsCore/gsConfig.h gismo/gsCore/gsConfig.h
        HINTS ${GISMO_HINTS}
        PATH_SUFFIXES build include
        PATHS
            /usr/local
            /usr
            /opt/gismo
    )

    if(GISMO_LIBRARY AND GISMO_INCLUDE_DIR)
        set(GISMO_FOUND TRUE)
        set(GISMO_INCLUDE_DIRS ${GISMO_INCLUDE_DIR})

        if(GISMO_EXTERNAL_DIR)
            list(APPEND GISMO_INCLUDE_DIRS ${GISMO_EXTERNAL_DIR})
        endif()
        if(GISMO_CONFIG_DIR AND NOT "${GISMO_CONFIG_DIR}" STREQUAL "${GISMO_INCLUDE_DIR}")
            list(APPEND GISMO_INCLUDE_DIRS ${GISMO_CONFIG_DIR})
        endif()

        message(STATUS "Found G+Smo: ${GISMO_LIBRARY}")
        message(STATUS "G+Smo includes: ${GISMO_INCLUDE_DIRS}")
    else()
        set(GISMO_FOUND FALSE)
        message(STATUS "G+Smo not found. Multipatch support disabled.")
        message(STATUS "  To enable: cmake -DGISMO_ROOT=/path/to/gismo ..")
        message(STATUS "  Or set environment variable GISMO_ROOT")
        message(STATUS "  Or disable: cmake -DWITH_GISMO=OFF ..")
    endif()
else()
    set(GISMO_FOUND FALSE)
    message(STATUS "G+Smo support disabled (WITH_GISMO=OFF)")
endif()

# GLAD (OpenGL loader - glad2 generates gl.c)
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/external/glad/src/gl.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/external/glad/include
)

# GLM (header-only)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    ${CMAKE_SOURCE_DIR}/external
)

# tinyobjloader (header-only implementation)
add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE
    ${CMAKE_SOURCE_DIR}/external/tinyobjloader
)

# Collect source files
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

# Main executable
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/glad/include
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/tinyobjloader
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    glad
    glm
    tinyobjloader
    OpenMP::OpenMP_CXX
)

# Link G+Smo if available
if(GISMO_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GISMO_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GISMO_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PRIVATE GISMO_AVAILABLE)
endif()

# Copy shaders to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl pthread)
endif()
