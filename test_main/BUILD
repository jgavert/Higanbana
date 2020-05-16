cc_binary(
        name = "test_main",
        srcs = glob(["**/*.cpp"]) + glob(["**/*.hpp"]),
        deps = ["//core:core", "//graphics:graphics", "//ext:cxxopts", "//ext:tinygltf"],
        copts = select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX", "/permissive-", "/await","-ftime-trace"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
        }),
        defines = ["_WIN64"],
        linkopts = select({
        "@bazel_tools//src/conditions:windows": ["/subsystem:WINDOWS"],
        "//conditions:default": ["-pthread"],
        }),  
)