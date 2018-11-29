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

	  float asfloat()
	  {
		  return static_cast<float>(value) / static_cast<float>(std::numeric_limits<int16_t>::max());
	  }
    };
    struct CSlider
    {
      uint16_t value;
	  float asfloat()
	  {
		  return static_cast<float>(value) / static_cast<float>(std::numeric_limits<uint16_t>::max());
	  }
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
    struct X360LikePad
    {
      bool alive = false;
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

	  bool isBeingUsed(const float deadzone = 0.15f)
	  {
		  // any button is pressed == is being used
		  bool buttons = dpad.down | dpad.left | dpad.right | dpad.up
						| xbLayout.a | xbLayout.b | xbLayout.x | xbLayout.y
				   		| xbLayout.l1 | xbLayout.l2 | xbLayout.r1 | xbLayout.r2
						| xbLayout.l3 | xbLayout.r3 | xbLayout.select | xbLayout.start;

		  bool lstickx = std::abs(lstick[0].asfloat()) > deadzone;
		  bool lsticky = std::abs(lstick[1].asfloat()) > deadzone;
		  bool rstickx = std::abs(rstick[0].asfloat()) > deadzone;
		  bool rsticky = std::abs(rstick[1].asfloat()) > deadzone;
		  bool lt = lTrigger.asfloat() > deadzone;
		  bool rt = rTrigger.asfloat() > deadzone;

		  // any analog true
		  return  buttons
			  || lstickx
			  || lsticky
			  || rstickx
			  || rsticky
			  || lt
			  || rt;
	  }

	  /*
	  bool operator==(const X360LikePad& other) const
	  {
		  auto comparison = memcmp(this, &other, sizeof(X360LikePad));
		  return comparison == 0;
	  }
	  */
    };

    // directinput definitions

    // platform independent guid, it was just 128bits so...
    struct Igd
    {
      uint64_t data[2];
    };

	struct Gamepad
	{
		X360LikePad current = {};
		X360LikePad before = {};
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
	  Gamepad pad;
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