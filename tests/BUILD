load(":macros.bzl", "src_graphics_test")
load(":macros.bzl", "src_core_test")
load(":macros.bzl", "src_graphics_test_with_header")
load(":macros.bzl", "src_core_test_with_header")

cc_library(
    name = "catch-main",
    srcs = ["main.cpp"],
    deps = ["//ext:Catch2"],
    linkstatic = True
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
src_graphics_test("device_creation")
src_graphics_test("resource_creation")
src_graphics_test_with_header("readback_future")

test_suite(
    name = "all-graphics-tests",
    tests = [
        "test_graphics_commandbuffer",
        "test_graphics_device_creation",
        "test_graphics_readback_future",
        "test_graphics_resource_creation"
    ]
)

test_suite(
    name = "all-tests",
    tests = [
        "all-core-tests",
        "all-graphics-tests"
    ]
)