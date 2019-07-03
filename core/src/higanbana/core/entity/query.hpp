#pragma once
#include "sparsetable.hpp"
#include "tagtable.hpp"
#include "../system/LBS.hpp"

namespace higanbana
{
  template<size_t size>
  Bitfield<size> intersect(Bitfield<size>& v0, Bitfield<size>& v1)
  {
    std::array<__m128i, size> result;
    std::array<__m128i, size>& vec0 = v0.data();
    std::array<__m128i, size>& vec1 = v1.data();
    __m128i* p = reinterpret_cast<__m128i*>(result.data());
    for (size_t i = 0; i < size; ++i)
    {
      _mm_store_si128(p+i,_mm_and_si128(vec0[i], vec1[i]));
    }
    return Bitfield<size>(result);
  }

  template<size_t size>
  inline void intersect_void(Bitfield<size>& v0, Bitfield<size>& v1)
  {
    std::array<__m128i, size>& vec0 = v0.data();
    std::array<__m128i, size>& vec1 = v1.data();
    for (size_t i = 0; i < size; ++i)
    {
      vec0[i] =_mm_and_si128(vec0[i], vec1[i]);
    }
  }

  template<size_t size>
  Bitfield<size> bitunion(Bitfield<size>& v0, Bitfield<size>& v1)
  {
    std::array<__m128i, size> result;
    std::array<__m128i, size>& vec0 = v0.data();
    std::array<__m128i, size>& vec1 = v1.data();
    __m128i* p = reinterpret_cast<__m128i*>(result.data());
    for (size_t i = 0; i < size; ++i)
    {
      _mm_store_si128(p+i,_mm_or_si128(vec0[i], vec1[i]));
    }
    return Bitfield<size>(result);
  }

  template<typename ...Args>
  inline decltype(auto) pack(Args& ... args)
  {
    return std::forward_as_tuple(args...);
  }

  // for_each
  struct pass
  {
    template<typename ...T> inline pass(T...){}
  };

  // call the lambda with correct parameters
  template<typename T,size_t size, size_t rsize>
  inline Bitfield<rsize>& extractBitfield(_SparseTable<T, size, rsize>& table)
  {
    return table.getBitfield();
  }

  // call the lambda with correct parameters
  template<typename T, size_t size>
  inline Bitfield<size>& extractBitfield(_TagTable<T, size>& table)
  {
	  return table.getBitfield();
  }

  template<typename Func, typename... Args>
  inline void applyBitfield(Func&& func, Args&&... args)
  {
    pass{ (func(extractBitfield(std::forward<Args>(args))),1)... };
  }

  template<typename Func, typename Tup, std::size_t... index>
  inline void invoke_apply_Bitfield(Func&& func, Tup&& tup,
      std::index_sequence<index...>)
  {
      applyBitfield(std::forward<Func>(func),
          std::get<index>(std::forward<Tup>(tup))...);
  }

  // needed so we can create our intersection
  template<typename Tup, typename Func>
  inline decltype(auto) for_each_bitfield(Tup&& tup, Func&& func)
  {
      constexpr auto Size =
        std::tuple_size<typename std::decay<Tup>::type>::value;
      invoke_apply_Bitfield(std::forward<Func>(func),
                          std::forward<Tup>(tup),
                          std::make_index_sequence<Size>{});
  }
  // call the lambda with correct parameters
  template<typename T,size_t size, size_t rsize>
  inline char* extractPtr(_SparseTable<T, size, rsize>& table)
  {
    return table.getPtr();
  }
  template<typename T,size_t size, size_t rsize>
  inline size_t extractStride(_SparseTable<T, size, rsize>& table)
  {
    return sizeof(T);
  }
  template<typename Func, typename... Args>
  inline void applyPtr(Func&& func, Args&&... args)
  {
    pass{ (func(extractPtr(std::forward<Args>(args)), extractStride(std::forward<Args>(args))),1)... };
  }
  template<typename Func, typename Tup, std::size_t... index>
  inline void invoke_apply_tablePtr(Func&& func, Tup&& tup,
      std::index_sequence<index...>)
  {
      applyPtr(std::forward<Func>(func),
          std::get<index>(std::forward<Tup>(tup))...);
  }
  // needed so we can create our intersection
  template<typename Tup, typename Func>
  inline decltype(auto) for_each_tableptr(Tup&& tup, Func&& func)
  {
      constexpr auto Size =
        std::tuple_size<typename std::decay<Tup>::type>::value;
      invoke_apply_tablePtr(std::forward<Func>(func),
                          std::forward<Tup>(tup),
                          std::make_index_sequence<Size>{});
  }

  // call the lambda with correct parameters
  template<typename T,size_t size, size_t rsize>
  inline T& getIndex(size_t& i, _SparseTable<T, size, rsize>& table)
  {
    return table.get(i);
  }

  template<typename Func, typename Tup, std::size_t... index>
  inline void callFuncWithIndex(
      size_t table_index,
      Func&& func,
      Tup&& tup,
      std::index_sequence<index...>)
  {
      func(table_index,
          getIndex(table_index, std::get<index>(std::forward<Tup>(tup)))...);
  }


  template<typename Tup, typename Func,
    size_t TupleSize = std::tuple_size<typename std::decay<Tup>::type>::value,
    size_t rsize = 2048,
    size_t innerArray = 256>
  void query(Tup&& tup, Func&& func)
  {
    Bitfield<rsize> intersected(true);
    for_each_bitfield(tup, [&](Bitfield<rsize>& b)
    {
      intersect_void(intersected, b);
    });

    size_t index = 0;
    size_t contiguous_fbs = 0;
    std::array<size_t, innerArray> idx_info;
    size_t idx = 0;
    while (index < rsize)
    {
      index = intersected.nextPopBucket(index);
      contiguous_fbs = intersected.contiguous_full_buckets(index);
      if (contiguous_fbs > 0)
      {
        for (size_t i = 0; i < contiguous_fbs * 128; ++i)
        {
          size_t id = index * 128 + i;
          callFuncWithIndex(id, std::forward<Func>(func),
                              std::forward<Tup>(tup),
                              std::make_index_sequence<TupleSize>{});
        }
        index += contiguous_fbs;
      }
      else
      {
        idx = 0;
        size_t temp_count = 0;
        while (index < rsize &&
            idx + (temp_count = intersected.popcount_element(index)) < innerArray+1)
        {
          if (temp_count == 0) // nothing here, next
          {
            ++index;
            continue;
          }
          else if (temp_count == 128) // Contiguous!! break and let the contiguous handle it.
          {                           // But keep the memory access linear so empty our array next.
            break;
          }
          idx = intersected.skip_find_indexes(idx_info, idx, index);
          ++index;
        }
        for (size_t i = 0; i < idx; ++i)
        {
          size_t id = idx_info[i];
          callFuncWithIndex(id, std::forward<Func>(func),
                              std::forward<Tup>(tup),
                              std::make_index_sequence<TupleSize>{});
        }
      }
    }
  }

 template<size_t rsize, typename Tup, typename Func,
    size_t TupleSize = std::tuple_size<typename std::decay<Tup>::type>::value,
    size_t innerArray = 128>
  void queryI(Tup&& tup, Func&& func)
  {
    Bitfield<rsize> intersected(true);
    for_each_bitfield(tup, [&](Bitfield<rsize>& b)
    {
      intersect_void(intersected, b);
    });

    size_t index = 0;
    size_t contiguous_fbs = 0;
    std::array<size_t, innerArray> idx_info;
    size_t idx = 0;
    while (index < rsize)
    {
      index = intersected.nextPopBucket(index);
      contiguous_fbs = intersected.contiguous_full_buckets(index);
      if (contiguous_fbs > 0)
      {
        for (size_t i = 0; i < contiguous_fbs * 128; ++i)
        {
          size_t id = index * 128 + i;
          callFuncWithIndex(id, std::forward<Func>(func),
                              std::forward<Tup>(tup),
                              std::make_index_sequence<TupleSize>{});
        }
        index += contiguous_fbs;
      }
      else
      {
        idx = 0;
        size_t temp_count = 0;
        while (index < rsize &&
            idx + (temp_count = intersected.popcount_element(index)) < innerArray+1)
        {
          if (temp_count == 0) // nothing here, next
          {
            ++index;
            continue;
          }
          else if (temp_count == 128) // Contiguous!! break and let the contiguous handle it.
          {                           // But keep the memory access linear so empty our array next.
            break;
          }
          idx = intersected.skip_find_indexes(idx_info, idx, index);
          ++index;
        }
        for (size_t i = 0; i < idx; ++i)
        {
          size_t id = idx_info[i];
          callFuncWithIndex(id, std::forward<Func>(func),
                              std::forward<Tup>(tup),
                              std::make_index_sequence<TupleSize>{});
        }
      }
    }
  }

  template<typename Tup, typename Tup2, typename Func,
	  size_t TupleSize = std::tuple_size<typename std::decay<Tup>::type>::value,
	  size_t rsize = 2048,
	  size_t innerArray = 256>
	  void query(Tup&& tup, Tup2&& tags, Func&& func)
  {
	  Bitfield<rsize> intersected(true);
	  for_each_bitfield(tup, [&](Bitfield<rsize>& b)
	  {
		  intersect_void(intersected, b);
	  });
	  for_each_bitfield(tags, [&](Bitfield<rsize>& b)
	  {
		  intersect_void(intersected, b);
	  });

	  size_t index = 0;
	  size_t contiguous_fbs = 0;
	  std::array<size_t, innerArray> idx_info;
	  size_t idx = 0;
	  while (index < rsize)
	  {
		  index = intersected.nextPopBucket(index);
		  contiguous_fbs = intersected.contiguous_full_buckets(index);
		  if (contiguous_fbs > 0)
		  {
			  for (size_t i = 0; i < contiguous_fbs * 128; ++i)
			  {
				  size_t id = index * 128 + i;
				  callFuncWithIndex(id, std::forward<Func>(func),
					  std::forward<Tup>(tup),
					  std::make_index_sequence<TupleSize>{});
			  }
			  index += contiguous_fbs;
		  }
		  else
		  {
			  idx = 0;
			  size_t temp_count = 0;
			  while (index < rsize &&
				  idx + (temp_count = intersected.popcount_element(index)) < innerArray + 1)
			  {
				  if (temp_count == 0) // nothing here, next
				  {
					  ++index;
					  continue;
				  }
				  else if (temp_count == 128) // Contiguous!! break and let the contiguous handle it.
				  {                           // But keep the memory access linear so empty our array next.
					  break;
				  }
				  idx = intersected.skip_find_indexes(idx_info, idx, index);
				  ++index;
			  }
			  for (size_t i = 0; i < idx; ++i)
			  {
				  size_t id = idx_info[i];
				  callFuncWithIndex(id, std::forward<Func>(func),
					  std::forward<Tup>(tup),
					  std::make_index_sequence<TupleSize>{});
			  }
		  }
	  }
  }

  template<size_t rsize, typename Tup, typename Tup2, typename Func,
	  size_t TupleSize = std::tuple_size<typename std::decay<Tup>::type>::value,
	  size_t innerArray = 128>
  void queryI(Tup&& tup, Tup2&& tags, Func&& func)
  {
	  Bitfield<rsize> intersected(true);
	  for_each_bitfield(tup, [&](Bitfield<rsize>& b)
	  {
		  intersect_void(intersected, b);
	  });
	  for_each_bitfield(tags, [&](Bitfield<rsize>& b)
	  {
		  intersect_void(intersected, b);
	  });

	  size_t index = 0;
	  size_t contiguous_fbs = 0;
	  std::array<size_t, innerArray> idx_info;
	  size_t idx = 0;
	  while (index < rsize)
	  {
		  index = intersected.nextPopBucket(index);
		  contiguous_fbs = intersected.contiguous_full_buckets(index);
		  if (contiguous_fbs > 0)
		  {
			  for (size_t i = 0; i < contiguous_fbs * 128; ++i)
			  {
				  size_t id = index * 128 + i;
				  callFuncWithIndex(id, std::forward<Func>(func),
					  std::forward<Tup>(tup),
					  std::make_index_sequence<TupleSize>{});
			  }
			  index += contiguous_fbs;
		  }
		  else
		  {
			  idx = 0;
			  size_t temp_count = 0;
			  while (index < rsize &&
				  idx + (temp_count = intersected.popcount_element(index)) < innerArray + 1)
			  {
				  if (temp_count == 0) // nothing here, next
				  {
					  ++index;
					  continue;
				  }
				  else if (temp_count == 128) // Contiguous!! break and let the contiguous handle it.
				  {                           // But keep the memory access linear so empty our array next.
					  break;
				  }
				  idx = intersected.skip_find_indexes(idx_info, idx, index);
				  ++index;
			  }
			  for (size_t i = 0; i < idx; ++i)
			  {
				  size_t id = idx_info[i];
				  callFuncWithIndex(id, std::forward<Func>(func),
					  std::forward<Tup>(tup),
					  std::make_index_sequence<TupleSize>{});
			  }
		  }
	  }
  }

  template<size_t rsize, typename Tup2, typename Func,
	  size_t innerArray = 128>
    void queryTag(Tup2&& tags, Func&& func)
  {
    Bitfield<rsize> intersected(true);
    for_each_bitfield(tags, [&](Bitfield<rsize>& b)
    {
      intersect_void(intersected, b);
    });

    size_t index = 0;
    size_t contiguous_fbs = 0;
    std::array<size_t, innerArray> idx_info;
    size_t idx = 0;
    while (index < rsize)
    {
      index = intersected.nextPopBucket(index);
      contiguous_fbs = intersected.contiguous_full_buckets(index);
      if (contiguous_fbs > 0)
      {
        for (size_t i = 0; i < contiguous_fbs * 128; ++i)
        {
          size_t id = index * 128 + i;
          func(id);
        }
        index += contiguous_fbs;
      }
      else
      {
        idx = 0;
        size_t temp_count = 0;
        while (index < rsize &&
          idx + (temp_count = intersected.popcount_element(index)) < innerArray + 1)
        {
          if (temp_count == 0) // nothing here, next
          {
            ++index;
            continue;
          }
          else if (temp_count == 128) // Contiguous!! break and let the contiguous handle it.
          {                           // But keep the memory access linear so empty our array next.
            break;
          }
          idx = intersected.skip_find_indexes(idx_info, idx, index);
          ++index;
        }
        for (size_t i = 0; i < idx; ++i)
        {
          size_t id = idx_info[i];
          func(id);
        }
      }
    }
  }
  
  template<typename Tup, typename Func,
    size_t TupleSize = std::tuple_size<typename std::decay<Tup>::type>::value,
    size_t rsize = 2048,
    size_t innerArray = 256,
    size_t offset = 128>
  void queryParallel(LBS& lbs, desc::Task& task, Tup&& tup, Func&& func)
  {
    auto intersected = std::make_shared<Bitfield<rsize>>(Bitfield<rsize>(true));
    for_each_bitfield(tup, [&](Bitfield<rsize>& b)
    {
      intersect_void(*intersected, b);
    });
    lbs.addParallelFor<1>(task, 0, rsize / offset, [=](size_t query_index)
    {
      size_t index = query_index * offset;
      size_t contiguous_fbs = 0;
      std::array<size_t, innerArray> idx_info;
      size_t idx = 0;
      while (index < query_index * offset + offset)
      {
        index = intersected->nextPopBucket(index);
        contiguous_fbs = intersected->contiguous_full_buckets(index);
        if (contiguous_fbs > 0)
        {
          contiguous_fbs = std::min(index - query_index * offset + offset, contiguous_fbs);
          for (size_t i = index*128; i < contiguous_fbs*128+index*128; ++i)
          {
            callFuncWithIndex(i, func,
                                tup,
                                std::make_index_sequence<TupleSize>{});
          }
          index += contiguous_fbs;
        }
        else
        {
          idx = 0;
          size_t temp_count = 0;
          while (index < query_index * offset + offset && idx + (temp_count = intersected->popcount_element(index)) < innerArray + 1)
          {

            if (temp_count == 0)
            {
              ++index;
              continue;
            }
            else if (temp_count == 128) // Contiguous!! break and let the contiguous handle it.
            {                           // rewrite contiguous here because logical.
              break;
            }
            idx = intersected->skip_find_indexes(idx_info, idx, index);
            ++index;
          }
          for (size_t i = 0; i < idx; ++i)
          {
            callFuncWithIndex(idx_info[i], func,
                                tup,
                                std::make_index_sequence<TupleSize>{});
          }
        }
      }
    });
  }

  template<typename Tup, typename Tup2, typename Func,
	  size_t TupleSize = std::tuple_size<typename std::decay<Tup>::type>::value,
	  size_t rsize = 2048,
	  size_t innerArray = 256,
	  size_t offset = 32>
	  void queryParallel(LBS& lbs, desc::Task& task, Tup&& tup, Tup2&& tags, Func&& func)
  {
	  auto intersected = std::make_shared<Bitfield<rsize>>(Bitfield<rsize>(true));
	  for_each_bitfield(tup, [&](Bitfield<rsize>& b)
	  {
		  intersect_void(*intersected, b);
	  });
	  for_each_bitfield(tags, [&](Bitfield<rsize>& b)
	  {
		  intersect_void(*intersected, b);
	  });

	  lbs.addParallelFor<1>(task, 0, rsize / offset, [=](size_t query_index, size_t)
	  {
		  size_t index = query_index * offset;
		  size_t contiguous_fbs = 0;
		  std::array<size_t, innerArray> idx_info;
		  size_t idx = 0;
		  while (index < query_index * offset + offset)
		  {
			  index = intersected->nextPopBucket(index);
			  contiguous_fbs = intersected->contiguous_full_buckets(index);
			  if (contiguous_fbs > 0)
			  {
				  contiguous_fbs = std::min(index - query_index * offset + offset, contiguous_fbs);
				  for (size_t i = index * 128; i < contiguous_fbs * 128 + index * 128; ++i)
				  {
					  callFuncWithIndex(i, func,
						  tup,
						  std::make_index_sequence<TupleSize>{});
				  }
				  index += contiguous_fbs;
			  }
			  else
			  {
				  idx = 0;
				  size_t temp_count = 0;
				  while (index < query_index * offset + offset && idx + (temp_count = intersected->popcount_element(index)) < innerArray + 1)
				  {

					  if (temp_count == 0)
					  {
						  ++index;
						  continue;
					  }
					  else if (temp_count == 128) // Contiguous!! break and let the contiguous handle it.
					  {                           // rewrite contiguous here because logical.
						  break;
					  }
					  idx = intersected->skip_find_indexes(idx_info, idx, index);
					  ++index;
				  }
				  for (size_t i = 0; i < idx; ++i)
				  {
					  callFuncWithIndex(idx_info[i], func,
						  tup,
						  std::make_index_sequence<TupleSize>{});
				  }
			  }
		  }
	  });
  }

  template<typename Tup, typename Func,
    size_t TupleSize = std::tuple_size<typename std::decay<Tup>::type>::value,
    size_t rsize = 2048,
    size_t innerArray = 128,
    size_t offset = 128>
  void queryParallel_prefetch(LBS& lbs, desc::Task& task, Tup&& tup, Func&& func)
  {
    auto intersected = std::make_shared<Bitfield<rsize>>(Bitfield<rsize>(true));
    for_each_bitfield(tup, [&](Bitfield<rsize>& b)
    {
      intersect_void(*intersected, b);
    });
    lbs.addParallelFor<1>(task, 0, rsize / offset, [=](size_t query_index, size_t)
    {
      size_t index = query_index * offset;
      size_t contiguous_fbs = 0;
      std::array<size_t, innerArray> idx_info;
      size_t idx = 0;
    
      while (index < query_index * offset + offset)
      {
        index = intersected->nextPopBucket(index);
        contiguous_fbs = intersected->contiguous_full_buckets(index);
        if (contiguous_fbs > 0)
        {
          contiguous_fbs = std::min(index - query_index * offset + offset, contiguous_fbs);
          for (size_t i = index*128; i < contiguous_fbs*128+index*128; ++i)
          {
            callFuncWithIndex(i, func,
                                tup,
                                std::make_index_sequence<TupleSize>{});
          }
          index += contiguous_fbs;
        }
        else
        {
          idx = 0;
          size_t temp_count = 0;
          while (index < query_index * offset + offset && idx + (temp_count = intersected->popcount_element(index)) < innerArray + 1)
          {

            if (temp_count == 0)
            {
              ++index;
              continue;
            }
            else if (temp_count == 128) // Contiguous!! break and let the contiguous handle it.
            {                           // rewrite contiguous here because logical.
              break;
            }
            idx = intersected->skip_find_indexes_prefetch(idx_info, idx, index, [tup](size_t index) 
            {
              for_each_tableptr(tup, [=, &index](char* ptr, size_t stride)
              {
#ifdef HIGANBANA_PLATFORM_WINDOWS
                _mm_prefetch(ptr + index*stride, _MM_HINT_NTA);
#else
                __builtin_prefetch(ptr + index*stride, 1, 0);
#endif
              });
            });
            ++index;
          }
          for (size_t i = 0; i < idx; ++i)
          {
            callFuncWithIndex(idx_info[i], func,
                                tup,
                                std::make_index_sequence<TupleSize>{});
          }
        }
      }
    });
  }
};


