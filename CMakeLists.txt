#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2025, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix.cpp - Threadpool Module
# ====================================================================
# Purpose:
#   Simple, modern, and observable multithreaded execution for Vix.
#   Provides developer-friendly APIs for thread pools, futures,
#   cancellation, priorities, timeouts, task groups, scopes, periodic
#   tasks, and parallel algorithms.
#
# Public Targets:
#   - vix_threadpool  : The actual library target
#   - vix::threadpool : Namespaced alias for consumers
#
# Installation/Export:
#   - Umbrella build exports into: VixTargets
#   - Standalone build exports into: vix_threadpoolTargets
# ====================================================================

cmake_minimum_required(VERSION 3.20)
project(vix_threadpool VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# --------------------------------------------------------------------
# Global settings
# --------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------
# Export set selection
# --------------------------------------------------------------------
if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(VIX_THREADPOOL_EXPORT_SET VixTargets)
else()
  set(VIX_THREADPOOL_EXPORT_SET vix_threadpoolTargets)
endif()

# --------------------------------------------------------------------
# Options
# --------------------------------------------------------------------
option(VIX_THREADPOOL_BUILD_EXAMPLES "Build threadpool examples" OFF)
option(VIX_THREADPOOL_BUILD_TESTS "Build threadpool tests" OFF)
option(VIX_THREADPOOL_BUILD_BENCHMARKS "Build threadpool benchmarks" OFF)

# --------------------------------------------------------------------
# Sources discovery
# --------------------------------------------------------------------
file(GLOB_RECURSE VIX_THREADPOOL_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# --------------------------------------------------------------------
# Library target
# --------------------------------------------------------------------
if (VIX_THREADPOOL_SOURCES)
  message(STATUS "[threadpool] Building STATIC library with detected sources.")

  add_library(vix_threadpool STATIC ${VIX_THREADPOOL_SOURCES})
  add_library(vix::threadpool ALIAS vix_threadpool)

  target_compile_features(vix_threadpool PUBLIC cxx_std_20)

  target_include_directories(vix_threadpool
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  find_package(Threads REQUIRED)

  target_link_libraries(vix_threadpool
    PUBLIC
      Threads::Threads
  )

  if (NOT MSVC)
    target_compile_options(vix_threadpool PRIVATE
      -Wall
      -Wextra
      -Wpedantic
    )
  endif()

  if (VIX_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(vix_threadpool PRIVATE
      -fno-omit-frame-pointer
      -fsanitize=address,undefined
    )

    target_link_options(vix_threadpool PRIVATE
      -fsanitize=address,undefined
    )
  endif()

  set_target_properties(vix_threadpool PROPERTIES
    OUTPUT_NAME vix_threadpool
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    EXPORT_NAME threadpool
  )

  install(TARGETS vix_threadpool
    EXPORT ${VIX_THREADPOOL_EXPORT_SET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

else()
  message(STATUS "[threadpool] Building HEADER-ONLY library (no sources).")

  add_library(vix_threadpool INTERFACE)
  add_library(vix::threadpool ALIAS vix_threadpool)

  target_compile_features(vix_threadpool INTERFACE cxx_std_20)

  target_include_directories(vix_threadpool INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  find_package(Threads REQUIRED)

  target_link_libraries(vix_threadpool INTERFACE
    Threads::Threads
  )

  set_target_properties(vix_threadpool PROPERTIES
    EXPORT_NAME threadpool
  )

  install(TARGETS vix_threadpool
    EXPORT ${VIX_THREADPOOL_EXPORT_SET}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

# --------------------------------------------------------------------
# Install headers
# --------------------------------------------------------------------
if (NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
      PATTERN "*.hpp"
      PATTERN "*.h"
  )
endif()

# --------------------------------------------------------------------
# Standalone package config/export
# Only for non-umbrella builds
# --------------------------------------------------------------------
if (NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vix_threadpoolConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vix_threadpoolConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_threadpool"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/vix_threadpoolConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/vix_threadpoolConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/vix_threadpoolConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_threadpool"
  )

  install(EXPORT vix_threadpoolTargets
    FILE vix_threadpoolTargets.cmake
    NAMESPACE vix::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vix_threadpool"
  )

endif()

# --------------------------------------------------------------------
# Examples
# --------------------------------------------------------------------
if (VIX_THREADPOOL_BUILD_EXAMPLES)
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
  endif()
endif()

# --------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------
if (VIX_THREADPOOL_BUILD_TESTS)
  include(CTest)
  enable_testing()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  endif()
endif()

# --------------------------------------------------------------------
# Benchmarks
# --------------------------------------------------------------------
if (VIX_THREADPOOL_BUILD_BENCHMARKS)
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
  endif()
endif()

# --------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------
message(STATUS "------------------------------------------------------")
message(STATUS "vix::threadpool configured (${PROJECT_VERSION})")
if (VIX_THREADPOOL_SOURCES)
  message(STATUS "Mode: STATIC / sources found")
else()
  message(STATUS "Mode: HEADER-ONLY / no sources")
endif()
message(STATUS "Export set: ${VIX_THREADPOOL_EXPORT_SET}")
message(STATUS "Include dir: ${CMAKE_CURRENT_SOURCE_DIR}/include (for <vix/threadpool/...>)")
message(STATUS "Build examples: ${VIX_THREADPOOL_BUILD_EXAMPLES}")
message(STATUS "Build tests: ${VIX_THREADPOOL_BUILD_TESTS}")
message(STATUS "Build benchmarks: ${VIX_THREADPOOL_BUILD_BENCHMARKS}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "------------------------------------------------------")
