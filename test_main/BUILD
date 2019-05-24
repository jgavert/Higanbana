

cc_binary(
    name = "test_main",
    srcs = glob(["**/*.cpp"]) + glob(["**/*.hpp"]),
    deps = ["//core:core", "//graphics:graphics"],
	copts = select({
        "@bazel_tools//src/conditions:windows": ["/std:c++latest"],
        "//conditions:default": ["-std=c++2a"],
	}),
	linkopts = select({
        "@bazel_tools//src/conditions:windows": ["/subsystem:WINDOWS"],
        "//conditions:default": [""],
	}),  
)