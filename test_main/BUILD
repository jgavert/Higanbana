cc_binary(
        name = "test_main",
        srcs = glob(["**/*.cpp"]) + glob(["**/*.hpp"]),
        deps = ["//core:core_coro", "//graphics:graphics_coro", "//ext/cxxopts:cxxopts", "//ext:cgltf"],
        copts = select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX2", "/permissive-", "/Z7", "-ftime-trace"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
        }),
        defines = ["_WIN64"],
        data = ["//data:mapping.json"],
        linkopts = select({
        "@bazel_tools//src/conditions:windows": ["/subsystem:WINDOWS", "/DEBUG"],
        "//conditions:default": ["-pthread"],
        }),  
)