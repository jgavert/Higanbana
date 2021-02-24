def src_graphics_test(target_name):
  native.cc_test(
    name = "test_graphics_" + target_name,
    srcs = ["graphics/test_" + target_name + ".cpp", "graphics/graphics_config.hpp", "graphics/graphics_config.cpp"],
    deps = ["//graphics:graphics", "//ext/Catch2:catch2_main"],
    copts = select({
      "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX2", "/Z7", "/permissive-"],
      "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
    data = ["//tests/data:mapping"],
    defines = ["_ENABLE_EXTENDED_ALIGNED_STORAGE"],
    linkopts = select({
      "@bazel_tools//src/conditions:windows": ["/subsystem:CONSOLE", "/DEBUG"],
      "//conditions:default": ["-pthread", "-ltbb", "-ldl"],
    }),
  )  

def src_graphics_test_with_header(target_name):
  native.cc_test(
    name = "test_graphics_" + target_name,
    srcs = ["graphics/test_" + target_name + ".cpp", "graphics/test_" + target_name + ".hpp", "graphics/graphics_config.hpp", "graphics/graphics_config.cpp"],
    deps = ["//graphics:graphics", "//ext/Catch2:catch2_main"],
    copts = select({
      "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX2", "/Z7", "/permissive-"],
      "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
    data = ["//tests/data:mapping"],
    defines = ["_ENABLE_EXTENDED_ALIGNED_STORAGE"],
    linkopts = select({
      "@bazel_tools//src/conditions:windows": ["/subsystem:CONSOLE", "/DEBUG"],
      "//conditions:default": ["-pthread", "-ltbb", "-ldl"],
    }),
  )  

def src_core_test(target_name):
  native.cc_test(
    name = "test_core_" + target_name,
    srcs = ["core/test_" + target_name + ".cpp"],
    deps = ["//core:core", "//ext/Catch2:catch2_main"],
    copts = select({
      "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX2", "/permissive-", "/Z7"],
      "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64", "-pthread"],
    }),
    data = ["//tests/data:mapping"],
    defines = ["_ENABLE_EXTENDED_ALIGNED_STORAGE"],
    linkopts = select({
      "@bazel_tools//src/conditions:windows": ["/subsystem:CONSOLE", "/DEBUG"],
      "//conditions:default": ["-pthread"],
    }),
  )  

def src_core_test_with_header(target_name):
  native.cc_test(
    name = "test_core_" + target_name,
    srcs = ["core/test_" + target_name + ".cpp", "core/test_" + target_name + ".hpp"],
    deps = ["//core:core", "//ext/Catch2:catch2_main"],
    copts = select({
      "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX2", "/permissive-", "/Z7"],
      "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
    data = ["//tests/data:mapping"],
    defines = ["_ENABLE_EXTENDED_ALIGNED_STORAGE"],
    linkopts = select({
      "@bazel_tools//src/conditions:windows": ["/subsystem:CONSOLE", "/DEBUG"],
      "//conditions:default": ["-pthread"],
    }),
  )  