load(":macros.bzl", "src_graphics_test")
load(":macros.bzl", "src_core_test")
load(":macros.bzl", "src_graphics_test_with_header")
load(":macros.bzl", "src_core_test_with_header")

src_core_test("entity")
src_core_test("tlsf")
src_core_test("tlsf_heap")
src_core_test("bitfield")
src_core_test("delta_buffer")
src_core_test("experimental_threading")
src_core_test("experimental_threading2")
src_core_test("camera_math")
src_core_test("radix_sort")

test_suite(
    name = "all-core-tests",
    tests = [
        "test_core_entity",
        "test_core_delta_buffer",
        "test_core_tlsf",
        "test_core_tlsf_heap",
        "test_core_experimental_threading",
        "test_core_experimental_threading2",
        "test_core_bitfield",
        "test_core_camera_math",
        "test_core_radix_sort"
    ]
)

src_graphics_test("commandbuffer")
src_graphics_test("device_creation")
src_graphics_test("resource_creation")
src_graphics_test_with_header("readback_future")
src_graphics_test("shader_matrix_math")
src_graphics_test("basics")
src_graphics_test("raytracing_basics")

test_suite(
    name = "all-graphics-tests",
    tests = [
        "test_graphics_basics",
        "test_graphics_commandbuffer",
        "test_graphics_device_creation",
        "test_graphics_readback_future",
        "test_graphics_resource_creation",
        "test_graphics_shader_matrix_math",
        "test_graphics_raytracing_basics"
    ]
)

test_suite(
    name = "all-tests",
    tests = [
        "all-core-tests",
        "all-graphics-tests"
    ]
)