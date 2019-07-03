#pragma once

#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <thread>
#include <functional>
#include <algorithm>

namespace higanbana
{

  template <typename T>
  class _FazMesg
  {
  public:
    class FazMesgListener
    {
    public:
      FazMesgListener() {
        _FazMesg<T>::Register(this);
      }
      ~FazMesgListener() {
        _FazMesg<T>::Remove(this);
      }
      virtual void handleFazMesg(T &t) = 0;
    };

    typedef std::list<FazMesgListener*> HandlerList;

    static void Remove(FazMesgListener* aListener)
    {
      registry.remove(aListener);
    }

    static void Register(FazMesgListener* aListener) {
      registry.push_back(aListener);
    }

    static void Notify(T* t) {
      for (FazMesgListener* i : registry) {
        i->handleFazMesg(*(T*)t);
      }
    }
    static HandlerList registry;
  private:
  };

  template <class T>
  void sendMessage(T& mesg)
  {
    //std::cout << "Sending mesg\n";
    _FazMesg<T>::Notify(&mesg);
  }


  template<typename T> typename _FazMesg<T>::HandlerList _FazMesg<T>::registry;

  template<typename T> class FazMesg : public  _FazMesg<T>::FazMesgListener {};

  // messagetypes
  /*
  struct NewTaskMessage // Used in threaded task system from inside the threads
  {
    NewTaskMessage(int threadID, std::function) :isDone([]() -> bool {return false; }), m_id(threadID)
    {

    }
    std::function<bool()> isDone;
    int m_id;
  };*/

  // test stuff messages
  struct viesti
  {
    int data;
  };

  struct LogMessage // possible memory leak
  {
    LogMessage(const char* data) : m_data(data) {}
    LogMessage(const LogMessage&) = delete;               //deleted copy constructor
    LogMessage& operator = (const LogMessage &) = delete; //deleted copy assignment operator
    const char* m_data;
  };
  /*
  namespace testmessage
  {
    // testcode
    class Test1 : public FazMesg<viesti>
    {
    public:
      Test1(std::string name) :m_name(name) {}
      ~Test1() {}
      void handleFazMesg(viesti &mesg)
      {
        mesg.data++;
        std::cout << "Moi olen " << m_name << std::endl;
      }
      std::string m_name;
    };

    class tester
    {
    public:
      void testTypedMessages()
      {
        Test1 test("keijo");
        Test1 test2("seppo");
        viesti woot;
        sendMessage(woot);
        sendMessage("woot");
      }
    };
  };*/

}