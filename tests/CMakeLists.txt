#  @file tests/CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2025, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix.cpp - Threadpool Tests
# ====================================================================

include(CTest)

# In umbrella builds, googletest may already be provided by the root
# CMakeLists.txt through FetchContent. Do not force a system GTest.
if (NOT TARGET GTest::gtest OR NOT TARGET GTest::gtest_main)
  find_package(GTest QUIET)
endif()

if (NOT TARGET GTest::gtest OR NOT TARGET GTest::gtest_main)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
  )

  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

if (NOT TARGET GTest::gtest OR NOT TARGET GTest::gtest_main)
  message(FATAL_ERROR "GTest targets are required to build vix::threadpool tests")
endif()

set(VIX_THREADPOOL_TEST_SOURCES
  ExecutorTest.cpp
  ParallelForEachTest.cpp
  ParallelForTest.cpp
  ParallelMapTest.cpp
  ParallelReduceTest.cpp
  PeriodicTaskTest.cpp
  ScopeTest.cpp
  ShutdownTest.cpp
  StressTest.cpp
  TaskCancellationTest.cpp
  TaskGroupTest.cpp
  TaskPriorityTest.cpp
  TaskQueueTest.cpp
  TaskTimeoutTest.cpp
  ThreadPoolConfigTest.cpp
  ThreadPoolMetricsTest.cpp
  ThreadPoolTest.cpp
  WorkerTest.cpp
)

foreach(test_source IN LISTS VIX_THREADPOOL_TEST_SOURCES)
  get_filename_component(test_name "${test_source}" NAME_WE)

  add_executable(${test_name}
    ${test_source}
  )

  target_compile_features(${test_name}
    PRIVATE
      cxx_std_20
  )

  target_link_libraries(${test_name}
    PRIVATE
      vix::threadpool
      GTest::gtest_main
  )

  if (NOT MSVC)
    target_compile_options(${test_name}
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
  endif()

  if (VIX_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${test_name}
      PRIVATE
        -fno-omit-frame-pointer
        -fsanitize=address,undefined
    )

    target_link_options(${test_name}
      PRIVATE
        -fsanitize=address,undefined
    )
  endif()

  add_test(
    NAME ${test_name}
    COMMAND ${test_name}
  )
endforeach()
