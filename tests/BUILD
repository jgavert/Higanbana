load(":macros.bzl", "src_graphics_test")
load(":macros.bzl", "src_core_test")

cc_library(
    name = "catch-main",
    srcs = ["main.cpp"],
    deps = ["//ext:Catch2"]
)

src_core_test("entity")
src_core_test("bitfield")

test_suite(
    name = "all-core-tests",
    tests = [
        "test_core_entity",
        "test_core_bitfield"
    ]
)

src_graphics_test("commandbuffer")

test_suite(
    name = "all-graphics-tests",
    tests = [
        "test_graphics_commandbuffer"
    ]
)

test_suite(
    name = "all-tests",
    tests = [
        "all-core-tests",
        "all-graphics-tests"
    ]
)