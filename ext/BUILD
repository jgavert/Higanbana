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