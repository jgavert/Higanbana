#pragma once

namespace higanbana
{
template <typename T, typename... Enums>
class EnumArray
{
  template<typename ... EnumTypes>
  struct EnumCount {
    static constexpr const size_t count = 1;
  };

  template<typename EnumType, typename... EnumTypes>
  struct EnumCount<EnumType, EnumTypes...>{
    static constexpr const size_t count = static_cast<size_t>(EnumType::Count) * EnumCount<EnumTypes...>::count;
  };

  static constexpr const size_t allEnumsCounts = EnumCount<Enums...>::count;
  T objs[allEnumsCounts];

  template<typename Enum>
  inline size_t calculateIndex(Enum val) const{
    return static_cast<size_t>(val);
  }

  template<typename Enum, typename... Enums2>
  inline size_t calculateIndex(Enum val, Enums2... rest) const {
    return static_cast<size_t>(val)*EnumCount<Enums2...>::count + calculateIndex(rest...);
  }

  public:
  T& operator()(Enums... vals) {
    const size_t index = calculateIndex(vals...);
    return objs[index];
  }
};
}