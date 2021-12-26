
def src_core_benchmark(target_name):
  native.cc_binary(
    name = "bench_core_" + target_name,
    srcs = ["core/bench_" + target_name + ".cpp"],
    deps = ["//core:core_coro", "//ext/Catch2:catch2_main"],
    copts = select({
      "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX2", "/permissive-", "/Z7"],
      "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64", "-pthread"],
    }),
    defines = ["_ENABLE_EXTENDED_ALIGNED_STORAGE", "CATCH_CONFIG_ENABLE_BENCHMARKING", "_HAS_DEPRECATED_RESULT_OF"],
    linkopts = select({
      "@bazel_tools//src/conditions:windows": ["/subsystem:CONSOLE", "/DEBUG"],
      "//conditions:default": ["-pthread"],
    }),
  )  
