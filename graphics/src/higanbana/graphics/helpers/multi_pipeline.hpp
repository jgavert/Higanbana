#pragma once
#include "higanbana/graphics/common/pipeline.hpp"
#include "higanbana/graphics/common/renderpass.hpp"
#include <tuple>
#include <utility>

namespace higanbana
{
template <typename T, typename... Enums>
class MultiObject
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

  template<typename Enum, typename... Enums>
  inline size_t calculateIndex(Enum val, Enums... rest) const {
    return static_cast<size_t>(val)*EnumCount<Enums...>::count + calculateIndex(rest...);
  }

  public:
  T& operator()(Enums... vals) {
    const size_t index = calculateIndex(vals...);
    return objs[index];
  }
};
  template<typename... Enums>
  using GraphicsPipelines = MultiObject<GraphicsPipeline, Enums...>;
  template<typename... Enums>
  using Renderpasses = MultiObject<Renderpass, Enums...>;
}

/*
MultiGraphicsPipeline<Enum1, Enum2> pipes;
pipes(Enum1::super, Enum2::ebin) = device.createPipeline yadayada
*/