

cc_binary(
        name = "test_main",
        srcs = glob(["**/*.cpp"]) + glob(["**/*.hpp"]),
        deps = ["//core:core", "//graphics:graphics"],
        copts = select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX", "/permissive-"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
        }),
        linkopts = select({
        "@bazel_tools//src/conditions:windows": ["/subsystem:WINDOWS"],
        "//conditions:default": [""],
        }),  
)