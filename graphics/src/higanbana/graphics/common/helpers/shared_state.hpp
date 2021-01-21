#pragma once

#include <memory>

namespace higanbana
{
namespace backend
{
template <typename State>
class SharedState
{
protected:
  using StatePtr = std::shared_ptr<State>;

  StatePtr m_state;
public:

  template <typename... Ts>
  void makeState(Ts&&... ts)
  {
    m_state = std::make_shared<State>(std::forward<Ts>(ts)...);
  }

  const State& S() const { return *m_state; }
  State& S() { return *m_state; }

  bool valid() const { return !!m_state; }
};
}
}
