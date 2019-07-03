cc_library(
    name = "STB",
    hdrs = ["stb/stb.h", "stb/stb_image.h"],
    includes = ["stb"],
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
    interface_library = "dxc-bin/lib/dxcompiler.lib",
    shared_library = "dxc-bin/bin/dxcompiler.dll",
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
     "winpixeventruntime/Include/WinPixEventRuntime/PIXEventsGenerated.h"],
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
    hdrs = ["Catch2/include/catch.hpp",
     "Catch2/include/catch_with_main.hpp",],
    includes = ["Catch2/include"],
    visibility = ["//visibility:public"], 
)

cc_library(
    name = "IMGui",
    srcs = ["imgui/imgui.cpp", "imgui/imgui_widgets.cpp", "imgui/imgui_draw.cpp", "imgui/imgui_demo.cpp"],
    hdrs = ["imgui/imgui.h", "imgui/imconfig.h", "imgui/imgui_internal.h", "imgui/imstb_rectpack.h", "imgui/imstb_textedit.h", "imgui/imstb_truetype.h"],
    includes = ["imgui"],
    visibility = ["//visibility:public"], 
)