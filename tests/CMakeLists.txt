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

find_package(GTest REQUIRED)

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

  target_link_libraries(${test_name}
    PRIVATE
      vix::threadpool
      GTest::gtest_main
  )

  target_compile_features(${test_name}
    PRIVATE
      cxx_std_20
  )

  if (NOT MSVC)
    target_compile_options(${test_name}
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
  endif()

  add_test(
    NAME ${test_name}
    COMMAND ${test_name}
  )
endforeach()
