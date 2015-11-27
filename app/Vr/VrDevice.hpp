#pragma once
#include "core/src/global_debug.hpp"
#include <OVR.h>

class Vrdevice
{

public:
  Vrdevice()
  {
    ovrResult result = ovr_Initialize(nullptr);
    if (!OVR_SUCCESS(result))
    {
      F_LOG("failed to initialize OculusVR\n", "");
    }
    ovrSession session;
    ovrGraphicsLuid luid;
    result = ovr_Create(&session, &luid);
    if (!OVR_SUCCESS(result))
    {
      F_LOG("failed to create session OculusVR\n", "");
      ovr_Shutdown();
    }
    else
    {

      ovrHmdDesc desc = ovr_GetHmdDesc(session);
      F_LOG("Found oculus device!\n");
      F_LOG("ProductName: %s\n", desc.ProductName);
      F_LOG("Manufacturer: %s\n", desc.Manufacturer);
      F_LOG("VendorId: %d\n", desc.VendorId);
      F_LOG("ProductId: %d\n", desc.ProductId);
      F_LOG("SerialNumber: %s\n", desc.SerialNumber);
      F_LOG("FirmwareMajor: %d\n", desc.FirmwareMajor);
      F_LOG("FirmwareMinor: %d\n", desc.FirmwareMinor);
      F_LOG("CameraFrustumHFovInRadians: %f\n", desc.CameraFrustumHFovInRadians);
      F_LOG("CameraFrustumVFovInRadians: %f\n", desc.CameraFrustumVFovInRadians);
      F_LOG("CameraFrustumNearZInMeters: %f\n", desc.CameraFrustumNearZInMeters);
      F_LOG("CameraFrustumNearZInMeters: %f\n", desc.CameraFrustumNearZInMeters);
      F_LOG("AvailableHmdCaps: %u\n", desc.AvailableHmdCaps);
      F_LOG("DefaultHmdCaps: %u\n", desc.DefaultHmdCaps);
      F_LOG("AvailableTrackingCaps: %u\n", desc.AvailableTrackingCaps);
      F_LOG("DefaultTrackingCaps: %u\n", desc.DefaultTrackingCaps);
      ovrSizei resolution = desc.Resolution;
      F_LOG("Resolution: %dx%d\n", resolution.w, resolution.h);
      F_LOG("DisplayRefreshRate: %f\n", desc.DisplayRefreshRate);

      ovr_Destroy(session);
      ovr_Shutdown();
    }
  }
};