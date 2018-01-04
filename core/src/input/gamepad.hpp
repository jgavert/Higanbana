#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace faze
{
  namespace gamepad
  {
    // gamepad definitions
    struct CAxis
    {
      int16_t value;
    };
    struct CSlider
    {
      uint16_t value;
    };
    struct CPov
    {
      bool up;
      bool down;
      bool left;
      bool right;
    };
    struct CButtons
    {
      std::vector<int> state;
    };

    struct XButtons
    {
      bool a, b, x, y; // 0, 1, 2 ,3
      bool l1, l2, l3, r1, r2, r3; // 4, 5
      bool select, start; // 6, 7
    };

    struct PSButtons
    {
      bool x, circle, box, triangle; // 0, 1, 2 ,3
      bool l1, l2, l3, r1, r2, r3; // 4, 5
      bool select, start; // 6, 7
    };

    // as some controllers are somewhat standardized by the amount of buttons
    // I will make assumption that I want mostly those layouts.
    // Think of, playstation controller or fighting stick (usually made to work with consoles)
    // XInput devices should be handled with different library :(.
    struct X360LikePad
    {
      // sticks
      CAxis lstick[2] = {}; // normal X/Y
      CAxis rstick[2] = {}; // rotation X/Y

      // dpad, usually pov
      CPov dpad;

      // triggers
      CSlider lTrigger;
      CSlider rTrigger;

      // buttons
      union
      {
        XButtons xbLayout;
        PSButtons psLayout;
      };
    };

    // directinput definitions

    // platform independent guid, it was just 128bits so...
    struct Igd
    {
      uint64_t data[2];
    };

    class InputDevice
    {
    public:
      enum class Type
      {
        Mouse,
        Keyboard,
        GameController,
        Unsupported
      };

      uint64_t updated;
      bool lost = false;
      Igd guid;
      Type type = Type::Unsupported;
      uintptr_t devPtr = 0;
      bool xinputDevice = false;
      std::string sguid;
      std::string name;
      X360LikePad pad;
    };

    struct Gamepad
    {
      bool alive = false;
      X360LikePad pad = {};
    };

    const char* toString(InputDevice::Type type);

    // minimal includes for Compile friendliness, especially leaking windows.h is not nice.
    class Fic // faze input controllers, directinput
    {
    public:
      enum class PollOptions
      {
        AllowDeviceEnumeration,
        OnlyPoll
      };
    private:
      uintptr_t m_DI = 0;
      uint64_t tickCounter = 0;
      bool m_usable = false;
      std::vector<InputDevice> m_seenDevices;
      std::unordered_set<std::string> m_ignoreList;
      std::unordered_map<std::string, InputDevice> m_devices;

      Gamepad xinput[4];

      bool m_seeminglyNoConnectedControllers = true;
    public:

      Fic();
      ~Fic();
      explicit operator bool() const;
      void enumerateDevices();
      void pollDevices(PollOptions option);

      X360LikePad getFirstActiveGamepad();
    };
  }
}