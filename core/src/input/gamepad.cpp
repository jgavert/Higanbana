#include "gamepad.hpp"
#if defined(FAZE_PLATFORM_WINDOWS)
#define DIRECTINPUT_VERSION 0x0800
#define _CRT_SECURE_NO_DEPRECATE
#ifndef _WIN32_DCOM
#define _WIN32_DCOM
#endif

#include <windows.h>

#pragma warning(push)
#pragma warning(disable:6000 28251)
#include <dinput.h>
#pragma warning(pop)

#include <dinputd.h>

#include <XInput.h>

#include "input_plat_tools.hpp"

#include <mutex>
#include <algorithm>

#include "core/src/global_debug.hpp"

#define INPUT_DEBUG 0

namespace faze
{
  namespace gamepad
  {
    static std::vector<InputDevice> g_deviceList;
    static std::unordered_set<std::string> g_ignoreList;
    static std::mutex g_ficGuard;

    const char* toString(InputDevice::Type type)
    {
      switch (type)
      {
      case InputDevice::Type::GameController:
        return "GameController";
      case InputDevice::Type::Keyboard:
        return "Keyboard";
      case InputDevice::Type::Mouse:
        return "Mouse";
      default:
        return "Unsupported";
      }
    }

    std::string guidToString(const GUID *guid) {
      char guid_string[37]; // 32 hex chars + 4 hyphens + null terminator
      snprintf(
        guid_string, sizeof(guid_string) / sizeof(guid_string[0]),
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2],
        guid->Data4[3], guid->Data4[4], guid->Data4[5],
        guid->Data4[6], guid->Data4[7]);
      return guid_string;
    }

    InputDevice::Type idevType(DWORD dwDevType)
    {
      auto type = dwDevType & 0xff;
      //auto subtype = dwDevType & 0xff00;
      switch (type)
      {
      case DI8DEVTYPE_1STPERSON:
      case DI8DEVTYPE_DRIVING:
      case DI8DEVTYPE_FLIGHT:
      case DI8DEVTYPE_GAMEPAD:
      case DI8DEVTYPE_JOYSTICK:
        return InputDevice::Type::GameController;
      case DI8DEVTYPE_KEYBOARD:
        return InputDevice::Type::Keyboard;
      case DI8DEVTYPE_MOUSE:
      case DI8DEVTYPE_SCREENPOINTER:
        return InputDevice::Type::Mouse;
      case DI8DEVTYPE_REMOTE:
      case DI8DEVTYPE_SUPPLEMENTAL:
      case DI8DEVTYPE_DEVICE:
      default:
        return InputDevice::Type::Unsupported;
      }
    }

    const char* devType(DWORD dwDevType)
    {
      auto type = dwDevType & 0xff;
      //auto subtype = dwDevType & 0xff00;
      switch (type)
      {
      case DI8DEVTYPE_1STPERSON:
        return "1st person device(!?)";
        //case DI8DEVTYPE1STPERSON_LIMITED:
        //case DI8DEVTYPE1STPERSON_SHOOTER:
        //case DI8DEVTYPE1STPERSON_SIXDOF:
        //case DI8DEVTYPE1STPERSON_UNKNOWN:

      case DI8DEVTYPE_DEVICE:
        return "Weird device";
        //case DI8DEVTYPE_DEVICECTRL:
        //case DI8DEVTYPEDEVICECTRL_COMMSSELECTION:
        //case DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED:
        //case DI8DEVTYPEDEVICECTRL_UNKNOWN:

      case DI8DEVTYPE_DRIVING:
        return "Driving device";
        //case DI8DEVTYPEDRIVING_COMBINEDPEDALS:
        //case DI8DEVTYPEDRIVING_DUALPEDALS:
        //case DI8DEVTYPEDRIVING_HANDHELD:
        //case DI8DEVTYPEDRIVING_LIMITED:
        //case DI8DEVTYPEDRIVING_THREEPEDALS:

      case DI8DEVTYPE_FLIGHT:
        return "Flight device";
        //case DI8DEVTYPEFLIGHT_LIMITED:
        //case DI8DEVTYPEFLIGHT_RC:
        //case DI8DEVTYPEFLIGHT_STICK:
        //case DI8DEVTYPEFLIGHT_YOKE:

      case DI8DEVTYPE_GAMEPAD:
        return "Flight device";
        //case DI8DEVTYPEGAMEPAD_LIMITED:
        //case DI8DEVTYPEGAMEPAD_STANDARD:
        //case DI8DEVTYPEGAMEPAD_TILT:

      case DI8DEVTYPE_JOYSTICK:
        return "Joystick device";
        //case DI8DEVTYPEJOYSTICK_LIMITED:
        //case DI8DEVTYPEJOYSTICK_STANDARD:

      case DI8DEVTYPE_KEYBOARD:
        return "Keyboard device";
      case DI8DEVTYPE_MOUSE:
        return "Mouse device";
      case DI8DEVTYPE_REMOTE:
        return "Remote device";
      case DI8DEVTYPE_SCREENPOINTER:
        return "Screenpointer device";
      case DI8DEVTYPE_SUPPLEMENTAL:
        return "Supplemental device";

      default:
        return "Unknown device";
      }
    }

    BOOL CALLBACK DIEnumDevicesCallback(
      LPCDIDEVICEINSTANCE lpddi,
      LPVOID
    )
    {
      if (g_ignoreList.find(guidToString(&lpddi->guidInstance)) != g_ignoreList.end())
      {
        return DIENUM_CONTINUE;
      }
      std::wstring prdct = lpddi->tszProductName;
      //std::string name = ws2s(prdct);
      auto sg = guidToString(&lpddi->guidInstance);
      //printf("id: %s ", sg.c_str());
      //printf(" type: %s ", devType(lpddi->dwDevType));
      //printf(" device: \"%s\"", name.c_str());
      std::wstring inst = lpddi->tszInstanceName;
      //auto instanceName = ws2s(inst);
      //printf(" instance: \"%s\"", instanceName.c_str());
      //printf("\n");

      InputDevice dev;
      dev.sguid = sg;
      memcpy(&dev.guid, &lpddi->guidInstance, sizeof(uint64_t) * 2);
      dev.name = ws2s(inst);
      dev.type = idevType(lpddi->dwDevType);

      dev.xinputDevice = IsXInputDevice(&lpddi->guidProduct);

      g_deviceList.emplace_back(dev);

      return DIENUM_CONTINUE;
    }

    //static int nSliderCount = 0;  // Number of returned slider controls
    //static int nPOVCount = 0;     // Number of returned POV controls

    BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID*)
    {
      //HWND hDlg = (HWND)pContext;

      // For axes that are returned, set the DIPROP_RANGE property for the
      // enumerated axis in order to scale min/max values.
      if (pdidoi->dwType & DIDFT_AXIS)
      {
        DIPROPRANGE diprg;
        diprg.diph.dwSize = sizeof(DIPROPRANGE);
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprg.diph.dwHow = DIPH_BYID;
        diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin = -1000;
        diprg.lMax = +1000;

        // Set the range for the axis
        //if (FAILED(g_pJoystick->SetProperty(DIPROP_RANGE, &diprg.diph)))
        //  return DIENUM_STOP;
      }
      std::string nam = ws2s(pdidoi->tszName);
      printf("name \"%s\" ", nam.c_str());

      printf(" {%d %d %d %d %d %d %d %d %d} ", pdidoi->dwDimension, pdidoi->dwFFForceResolution, pdidoi->dwFFMaxForce, pdidoi->dwFlags, pdidoi->dwOfs, pdidoi->wCollectionNumber, pdidoi->wDesignatorIndex, pdidoi->wReportId, pdidoi->wUsagePage);

      if (pdidoi->guidType == GUID_XAxis)
      {
        printf("found XAxis");
      }
      else if (pdidoi->guidType == GUID_YAxis)
      {
        printf("found YAxis");
      }
      else if (pdidoi->guidType == GUID_ZAxis)
      {
        printf("found ZAxis");
      }
      else if (pdidoi->guidType == GUID_RxAxis)
      {
        printf("found RxAxis");
      }
      else if (pdidoi->guidType == GUID_RyAxis)
      {
        printf("found RyAxis");
      }
      else if (pdidoi->guidType == GUID_RzAxis)
      {
        printf("found RzAxis");
      }
      else if (pdidoi->guidType == GUID_Slider)
      {
        printf("found Slider");
      }
      else if (pdidoi->guidType == GUID_POV)
      {
        printf("found POV");
      }
      else if (pdidoi->guidType == GUID_Button)
      {
        printf("found Button");
      }
      else if (pdidoi->guidType == GUID_Key)
      {
        printf("found Key");
      }
      printf("\n");

      return DIENUM_CONTINUE;
    }

    void updatePadState(X360LikePad& pad, DIJOYSTATE2& state)
    {
      int maxVal = std::numeric_limits<int16_t>::max();
      pad.lstick[0].value = static_cast<int16_t>(std::min(static_cast<int>(state.lX) - maxVal, maxVal));
      pad.lstick[1].value = static_cast<int16_t>(std::min(static_cast<int>(state.lY) - maxVal, maxVal) * -1);
      pad.rstick[0].value = static_cast<int16_t>(std::min(static_cast<int>(state.lZ) - maxVal, maxVal));
      pad.rstick[1].value = static_cast<int16_t>(std::min(static_cast<int>(state.lRz) - maxVal, maxVal) * -1);
      pad.lTrigger.value = static_cast<uint16_t>(state.lRx);
      pad.rTrigger.value = static_cast<uint16_t>(state.lRy);

      BOOL POVCentered = (LOWORD(state.rgdwPOV[0]) == 0xFFFF);
      pad.dpad.up = false;
      pad.dpad.down = false;
      pad.dpad.left = false;
      pad.dpad.right = false;
      if (!POVCentered)
      {
        auto hat = state.rgdwPOV[0];
        if (hat == 0 || hat == 36000 || hat == 4500 || hat == 36000 - 4500)
        {
          pad.dpad.up = true;
        }
        if (hat == 18000 || hat == 18000 - 4500 || hat == 18000 + 4500)
        {
          pad.dpad.down = true;
        }
        if (hat == 9000 || hat == 9000 - 4500 || hat == 9000 + 4500)
        {
          pad.dpad.right = true;
        }
        if (hat == 27000 || hat == 27000 - 4500 || hat == 27000 + 4500)
        {
          pad.dpad.left = true;
        }
      }

      pad.psLayout.box = state.rgbButtons[0];
      pad.psLayout.x = state.rgbButtons[1];
      pad.psLayout.circle = state.rgbButtons[2];
      pad.psLayout.triangle = state.rgbButtons[3];
      pad.psLayout.l1 = state.rgbButtons[4];
      pad.psLayout.r1 = state.rgbButtons[5];
      pad.psLayout.l2 = state.rgbButtons[6];
      pad.psLayout.r2 = state.rgbButtons[7];
      pad.psLayout.start = state.rgbButtons[8];
      pad.psLayout.select = state.rgbButtons[9];
      pad.psLayout.l3 = state.rgbButtons[10];
      pad.psLayout.r3 = state.rgbButtons[11];
#if INPUT_DEBUG
      printf("X-axis: %d ", pad.lstick[0].value);
      printf("Y-axis: %d ", pad.lstick[1].value);
      printf("RX-axis: %d ", pad.rstick[0].value);
      printf("RY-axis: %d ", pad.rstick[1].value);
      printf("ltrigger: %zd ", state.lRx);
      printf("rtrigger: %zd ", state.lRy);
      printf("dpad: %zd ", state.rgdwPOV[0]);
      printf("B: ");
      for (int i = 0; i < 14; ++i)
      {
        printf("%d: \"%d\" ", i, state.rgbButtons[i]);
      }
      printf("\n");
#endif
    }

    LPDIRECTINPUT8 getDIptr(uintptr_t ptr)
    {
      return reinterpret_cast<LPDIRECTINPUT8>(ptr);
    }

    LPDIRECTINPUTDEVICE8 getDevPtr(uintptr_t ptr)
    {
      return reinterpret_cast<LPDIRECTINPUTDEVICE8>(ptr);
    }

    Fic::Fic()
    {
      HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(&m_DI), nullptr);
      if (FAILED(hr))
      {
        printf("failed to initialize direct input\n");
      }
      else
      {
        m_usable = true;
        enumerateDevices();
      }
    }

    void Fic::enumerateDevices()
    {
      if (m_DI == 0)
        return;

      tickCounter++;
      auto ptr = getDIptr(m_DI);
      {
        std::lock_guard<std::mutex> guard(g_ficGuard);
        g_deviceList.clear();
        g_ignoreList = m_ignoreList;
        HRESULT hr = ptr->EnumDevices(DI8DEVCLASS_ALL, DIEnumDevicesCallback, 0, DIEDFL_ALLDEVICES);
        if (FAILED(hr))
        {
          printf("device enumeration failed\n");
          return;
        }
        m_seenDevices = g_deviceList;
        g_ignoreList.clear();
      }
      for (auto&& dev : m_seenDevices)
      {
        auto fdev = m_devices.find(dev.sguid);
        if (fdev == m_devices.end())
        {
          // new device!
          dev.updated = tickCounter;
          printf("New device: %s type: %s\n", dev.name.c_str(), toString(dev.type));
          m_ignoreList.insert(dev.sguid);
          if (dev.type == InputDevice::Type::GameController && !dev.xinputDevice)
          {
            GUID desired;
            memcpy(&desired, &dev.guid, sizeof(uint64_t) * 2);
            HRESULT hr = ptr->CreateDevice(desired, reinterpret_cast<LPDIRECTINPUTDEVICE8*>(&dev.devPtr), nullptr);
            if (FAILED(hr))
            {
              printf("Device creation failed\n");
            }
            else
            {
              if (SUCCEEDED(hr = getDevPtr(dev.devPtr)->SetDataFormat(&c_dfDIJoystick2)))
              {
                printf("Usable gamecontroller created (assume x360) %s\n", dev.name.c_str());

                if (FAILED(hr = getDevPtr(dev.devPtr)->SetCooperativeLevel(NULL, DISCL_EXCLUSIVE |
                  DISCL_FOREGROUND)))
                {
                  printf("oh no cooperation\n");
                }

                if (FAILED(hr = getDevPtr(dev.devPtr)->EnumObjects(EnumObjectsCallback, nullptr, DIDFT_ALL)))
                {
                  printf("oh no enumObjects\n");
                }
              }
            }
          }
          m_devices[dev.sguid] = dev;
        }
        else
        {
          fdev->second.updated = tickCounter;
          if (fdev->second.lost)
          {
            printf("Reappeared device: %s type: %s\n", dev.name.c_str(), toString(dev.type));
            fdev->second.lost = false;
          }
        }
      }
      for (auto&& dev : m_devices)
      {
        if (!dev.second.lost && tickCounter - dev.second.updated > 0)
        {
          // this device is lost.
          //printf("Lost device: %s type: %s\n", dev.second.name.c_str(), toString(dev.second.type));
          //dev.second.lost = true;
        }
      }
    }

    void Fic::pollDevices(PollOptions option)
    {
      if (m_seeminglyNoConnectedControllers && option == PollOptions::AllowDeviceEnumeration)
      {
        enumerateDevices();
      }
      for (auto&& dev : m_devices)
      {
        if (dev.second.devPtr != 0)
        {
          HRESULT hr = getDevPtr(dev.second.devPtr)->Poll();
          if (FAILED(hr) || hr == DIERR_INPUTLOST)
          {
            hr = getDevPtr(dev.second.devPtr)->Acquire();
          }
          if (SUCCEEDED(hr))
          {
            DIJOYSTATE2 js{};
            hr = getDevPtr(dev.second.devPtr)->GetDeviceState(sizeof(DIJOYSTATE2), &js);
            if (SUCCEEDED(hr))
            {
              updatePadState(dev.second.pad, js);
              dev.second.lost = false;
            }
          }
          else
          {
            if (!dev.second.lost)
            {
              printf("Lost device: %s type: %s\n", dev.second.name.c_str(), toString(dev.second.type));
              dev.second.lost = true;
            }
          }
        }
      }

      // xinput
      HRESULT hr = CoInitialize(NULL);
      bool bCleanupCOM = SUCCEEDED(hr);
      for (int i = 0; i < 4; ++i)
      {
        xinput[i].alive = false;

        XINPUT_STATE state{};
        auto res = XInputGetState(i, &state);
        if (ERROR_SUCCESS == res)
        {
          auto& pad = xinput[i].pad;
          xinput[i].alive = true;

          pad.lstick[0].value = state.Gamepad.sThumbLX;
          pad.lstick[1].value = state.Gamepad.sThumbLY;
          pad.rstick[0].value = state.Gamepad.sThumbRX;
          pad.rstick[1].value = state.Gamepad.sThumbRY;
          pad.lTrigger.value = static_cast<uint16_t>(state.Gamepad.bLeftTrigger) * 257;
          pad.rTrigger.value = static_cast<uint16_t>(state.Gamepad.bRightTrigger) * 257;

          pad.xbLayout.l2 = state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
          pad.xbLayout.r2 = state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

          auto isPressed = [&](int val) -> bool
          {
            auto thing = state.Gamepad.wButtons & val;
            bool what = thing == val;
            return what;
          };

          pad.xbLayout.a = isPressed(XINPUT_GAMEPAD_A);
          pad.xbLayout.b = isPressed(XINPUT_GAMEPAD_B);
          pad.xbLayout.x = isPressed(XINPUT_GAMEPAD_X);
          pad.xbLayout.y = isPressed(XINPUT_GAMEPAD_Y);
          pad.xbLayout.r1 = isPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER);
          pad.xbLayout.l1 = isPressed(XINPUT_GAMEPAD_LEFT_SHOULDER);
          pad.xbLayout.r3 = isPressed(XINPUT_GAMEPAD_RIGHT_THUMB);
          pad.xbLayout.l3 = isPressed(XINPUT_GAMEPAD_LEFT_THUMB);
          pad.xbLayout.start = isPressed(XINPUT_GAMEPAD_START);
          pad.xbLayout.select = isPressed(XINPUT_GAMEPAD_BACK);

          pad.dpad.up = isPressed(XINPUT_GAMEPAD_DPAD_UP);
          pad.dpad.down = isPressed(XINPUT_GAMEPAD_DPAD_DOWN);
          pad.dpad.left = isPressed(XINPUT_GAMEPAD_DPAD_LEFT);
          pad.dpad.right = isPressed(XINPUT_GAMEPAD_DPAD_RIGHT);
        }
        else
        {
          auto& pad = xinput[i].pad;
          pad = X360LikePad{};
        }
      }
      if (bCleanupCOM)
        CoUninitialize();
    }

    Fic::~Fic()
    {
      for (auto&& dev : m_devices)
      {
        if (dev.second.devPtr != 0)
        {
          getDevPtr(dev.second.devPtr)->Unacquire();
          getDevPtr(dev.second.devPtr)->Release();
          dev.second.devPtr = 0;
        }
      }
      if (m_DI)
      {
        getDIptr(m_DI)->Release();
        m_DI = 0;
      }
    }

    Fic::operator bool() const
    {
      return m_usable;
    }

    X360LikePad Fic::getFirstActiveGamepad()
    {
      m_seeminglyNoConnectedControllers = false;
      for (auto&& dev : m_devices)
      {
        if (dev.second.devPtr != 0 && !dev.second.lost)
        {
          return dev.second.pad;
        }
      }
      for (int i = 0; i < 4; ++i)
      {
        if (xinput[i].alive)
          return xinput[i].pad;
      }

      m_seeminglyNoConnectedControllers = true;
      auto pad = X360LikePad{};
      pad.alive = false;
      return pad;
    }
  }
}
#else

namespace faze
{
  namespace gamepad
  {
    Fic::Fic()
    {
    }

    void Fic::enumerateDevices()
    {
    }

    void Fic::pollDevices(PollOptions)
    {
    }

    Fic::~Fic()
    {
    }

    Fic::operator bool() const
    {
      return m_usable;
    }

    X360LikePad Fic::getFirstActiveGamepad()
    {
      auto pad = X360LikePad{};
      pad.alive = false;
      return pad;
    }
  }
}
#endif