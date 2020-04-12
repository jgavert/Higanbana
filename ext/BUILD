cc_library(
    name = "STB",
    hdrs = ["stb/stb.h", "stb/stb_image.h"],
    includes = ["stb"],
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "tinygltf",
    hdrs = ["tinygltf/stb_image.h", "tinygltf/stb_image_write.h", "tinygltf/json.hpp", "tinygltf/tiny_gltf.h"],
    includes = ["tinygltf"],
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "GSL",
    hdrs = ["GSL/include/gsl/gsl",
    "GSL/include/gsl/gsl_algorithm",
    "GSL/include/gsl/gsl_assert",
    "GSL/include/gsl/gsl_byte",
    "GSL/include/gsl/gsl_util",
    "GSL/include/gsl/multi_span",
    "GSL/include/gsl/pointers",
    "GSL/include/gsl/span",
    "GSL/include/gsl/string_span"],
    includes = ["GSL/include"],
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "nlohmann_json",
    hdrs = ["nlohmann_json/single_include/nlohmann/json.hpp"],
    includes = ["nlohmann_json/single_include/"],
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "DXC_headers",
    hdrs = ["dxc-bin/include/dxc/dxcapi.h",
     "dxc-bin/include/dxc/Support/microcom.h",
     "dxc-bin/include/llvm/Support/Atomic.h"],
    includes = ["dxc-bin/include"],
    linkstatic = False,
    visibility = ["//visibility:public"], 
)

cc_import(
    name = "DXC_lib",
    interface_library = select({
          "@bazel_tools//src/conditions:windows": "dxc-bin/lib/dxcompiler.lib",
          "//conditions:default": "dxc-bin/lib/libdxcompiler.so"}),
    shared_library = select({
          "@bazel_tools//src/conditions:windows": "dxc-bin/lib/dxcompiler.dll",
          "//conditions:default": "dxc-bin/lib/libdxcompiler.so"}),
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "DirectXShaderCompiler",
    deps = [
         "DXC_headers",
         "DXC_lib"
    ],
    linkstatic = False,
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "WinPixEventRuntime_headers",
    hdrs = ["winpixeventruntime/Include/WinPixEventRuntime/pix3.h",
     "winpixeventruntime/Include/WinPixEventRuntime/pix3_win.h",
     "winpixeventruntime/Include/WinPixEventRuntime/PIXEventsCommon.h",
     "winpixeventruntime/Include/WinPixEventRuntime/PIXEvents.h"],
    includes = ["winpixeventruntime/Include"],
    linkstatic = False,
    visibility = ["//visibility:public"], 
)

cc_import(
    name = "WinPixEventRuntime_lib",
    interface_library = "winpixeventruntime/bin/x64/WinPixEventRuntime.lib",
    shared_library = "winpixeventruntime/bin/x64/WinPixEventRuntime.dll",
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "WinPixEventRuntime",
    deps = [
         "WinPixEventRuntime_headers",
         "WinPixEventRuntime_lib"
    ],
    linkstatic = False,
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "Catch2",
    hdrs = ["Catch2/single_include/catch2/catch.hpp"],
    includes = ["Catch2/single_include"],
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "IMGui",
    srcs = ["imgui/imgui.cpp", "imgui/imgui_widgets.cpp", "imgui/imgui_draw.cpp", "imgui/imgui_demo.cpp"],
    hdrs = ["imgui/imgui.h", "imgui/imconfig.h", "imgui/imgui_internal.h", "imgui/imstb_rectpack.h", "imgui/imstb_textedit.h", "imgui/imstb_truetype.h"],
    includes = ["imgui"],
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "cxxopts",
    hdrs = ["cxxopts/include/cxxopts.hpp"],
    includes = ["cxxopts/include"],
    visibility = ["//visibility:public"], 
)
