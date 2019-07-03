#pragma once
#ifdef HIGANBANA_PLATFORM_WINDOWS
#include <renderdoc_app.h>
#endif

namespace higanbana
{
  class RenderDocApi
  {
#ifdef HIGANBANA_PLATFORM_WINDOWS
    RENDERDOC_API_1_0_0 *rdoc_api = nullptr;
#else
    int rdoc_api = 0;
#endif
  public:
    RenderDocApi()
    {
#ifdef HIGANBANA_PLATFORM_WINDOWS
      if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
      {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");

        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, (void **)&rdoc_api);

        F_ASSERT(ret == 1, "");
        F_ASSERT(rdoc_api->StartFrameCapture != nullptr && rdoc_api->EndFrameCapture != nullptr, "");

        int major = 999, minor = 999, patch = 999;

        rdoc_api->GetAPIVersion(&major, &minor, &patch);

        F_ASSERT(major == 1 && minor >= 0 && patch >= 0, "");
      }
#endif
    }

    void startCapture()
    {
#ifdef HIGANBANA_PLATFORM_WINDOWS
      if (rdoc_api)
      {
        rdoc_api->StartFrameCapture(nullptr, nullptr);
      }
#endif
    }

    void endCapture()
    {
#ifdef HIGANBANA_PLATFORM_WINDOWS
      if (rdoc_api)
      {
        rdoc_api->EndFrameCapture(nullptr, nullptr);
      }
#endif
    }
  };
};
