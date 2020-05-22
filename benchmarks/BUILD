
load(":macros.bzl", "src_core_benchmark")

cc_library(
    name = "catch-benchmark-main",
    srcs = ["main.cpp"],
    deps = ["//ext/Catch2:catch2"],
    defines = ["CATCH_CONFIG_ENABLE_BENCHMARKING"],
    linkstatic = True
)

src_core_benchmark("simple")
src_core_benchmark("coroutine")
