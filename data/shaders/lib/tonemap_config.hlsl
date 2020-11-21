#ifndef __tonemap_config__
#define __tonemap_config__

#ifdef HIGANBANA_DX12
// doesn't compile on vulkan :D
// so useful to be able to define it out from vulkan :DD
// amazing.
// TODO: make repro of tonemapping shader for DXC spirv not compiling
//#define ACEScg_RENDERING
//#define ACES_ENABLED
#endif
#define TEMP_TONEMAP
#endif // __tonemap_config__