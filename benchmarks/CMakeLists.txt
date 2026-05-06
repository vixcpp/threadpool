#  @file benchmarks/CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2025, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix.cpp - Threadpool Benchmarks
# ====================================================================

set(VIX_THREADPOOL_BENCHMARK_SOURCES
  submit_bench.cpp
  parallel_for_bench.cpp
  parallel_map_bench.cpp
  queue_contention_bench.cpp
  shutdown_bench.cpp
)

foreach(benchmark_source IN LISTS VIX_THREADPOOL_BENCHMARK_SOURCES)
  get_filename_component(benchmark_name "${benchmark_source}" NAME_WE)

  add_executable(${benchmark_name}
    ${benchmark_source}
  )

  target_link_libraries(${benchmark_name}
    PRIVATE
      vix::threadpool
  )

  target_compile_features(${benchmark_name}
    PRIVATE
      cxx_std_20
  )

  if (NOT MSVC)
    target_compile_options(${benchmark_name}
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
  endif()
endforeach()
