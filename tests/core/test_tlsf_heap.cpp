#include <catch2/catch.hpp>
#include <higanbana/core/platform/definitions.hpp>
#include <higanbana/core/allocators/heap_allocator_raw.hpp>
#include <css/utils/dynamic_allocator.hpp>

TEST_CASE("some basic allocation tests") {
  constexpr size_t size = 1024;
  void* heap = malloc(size);
  higanbana::HeapAllocatorRaw tlsf(heap, size);
  auto block = tlsf.allocate(512);
  REQUIRE(block);
  auto block2 = tlsf.allocate(512);
  REQUIRE_FALSE(block2);
  tlsf.free(block);
  block = tlsf.allocate(128);
  REQUIRE(block);
  block2 = tlsf.allocate(512);
  REQUIRE(block2);
  tlsf.free(block);
  block = tlsf.allocate(256);
  REQUIRE(block);
  tlsf.free(block);
  tlsf.free(block2);

  block = tlsf.allocate(1024 - 16);
  REQUIRE(block);

  free(heap);
}

TEST_CASE("some basic allocation tests 2") {
  constexpr size_t size = 70000;
  void* heap = malloc(size);
  higanbana::HeapAllocatorRaw tlsf(heap, size);
  auto block = tlsf.allocate(50000);
  REQUIRE(block);
  auto block2 = tlsf.allocate(20000);
  REQUIRE(block2 == nullptr);
  block2 = tlsf.allocate(20000 - 32);
  REQUIRE(block2);
  free(heap);
}

TEST_CASE("some basic allocation tests 3") {
  constexpr size_t size = 80000;
  css::DynamicHeapAllocator tlsf(1, size);

  auto writeTrash = [](void* ptr, size_t size) {
    memset(ptr, 254, size);
  };

  for (int i = 0; i < 2; i++) {
    auto block = tlsf.allocate(316);
    writeTrash(block, 316);
    REQUIRE(block);
    {
      std::vector<void*> ptrs;
      for (int k = 0; k < 5; ++k)
      {
        auto ptr = tlsf.allocate(216);
        writeTrash(ptr, 216);
        ptrs.push_back(ptr);
      }
      for (int k = 0; k < 5; ++k)
        tlsf.deallocate(ptrs[k]);
    }
    tlsf.deallocate(block);
  }
}

TEST_CASE("some basic allocation tests 4") {
  constexpr size_t size = 80000;
  css::DynamicHeapAllocator tlsf(1, size);

  auto writeTrash = [](void* ptr, size_t size) {
    memset(ptr, 254, size);
  };

  auto block = tlsf.allocate(14912);
  writeTrash(block, 14912);
  REQUIRE(block);
  auto block2 = tlsf.allocate(7240);
  writeTrash(block2, 7240);
  REQUIRE(block2);
  {
    std::vector<void*> ptrs;
    for (int k = 0; k < 4; ++k)
    {
      auto ptr = tlsf.allocate(2168);
      writeTrash(ptr, 2168);
      ptrs.push_back(ptr);
    }
    for (int k = 0; k < 4; ++k)
      tlsf.deallocate(ptrs[k]);
  }
  auto block3 = tlsf.allocate(3512);
  writeTrash(block3, 3512);
  REQUIRE(block3);
  auto block4 = tlsf.allocate(104);
  writeTrash(block4, 104);
  REQUIRE(block4);
  auto block5 = tlsf.allocate(696);
  writeTrash(block5, 696);
  REQUIRE(block5);
  //tlsf.deallocate(block5);
  auto block6 = tlsf.allocate(696);
  writeTrash(block6, 696);
  REQUIRE(block6);
  auto block7 = tlsf.allocate(368);
  writeTrash(block7, 368);
  REQUIRE(block7);
  auto block8 = tlsf.allocate(696);
  writeTrash(block8, 696);
  REQUIRE(block8);
  auto block9 = tlsf.allocate(128);
  writeTrash(block9, 128);
  REQUIRE(block9);
  tlsf.deallocate(block9);
  tlsf.deallocate(block8);
  tlsf.deallocate(block7);
  tlsf.deallocate(block6);
  tlsf.deallocate(block4);
  tlsf.deallocate(block3);
  tlsf.deallocate(block2);
  tlsf.deallocate(block);
  block = tlsf.allocate(14912);
  writeTrash(block, 14912);
  REQUIRE(block);
  block2 = tlsf.allocate(7240);
  writeTrash(block2, 7240);
  REQUIRE(block2);
  {
    std::vector<void*> ptrs;
    for (int k = 0; k < 4; ++k)
    {
      auto ptr = tlsf.allocate(2168);
      writeTrash(ptr, 2168);
      ptrs.push_back(ptr);
    }
    for (int k = 0; k < 4; ++k)
      tlsf.deallocate(ptrs[k]);
  }
}

TEST_CASE("some basic allocation tests 5") {
  constexpr size_t size = 80000;
  css::DynamicHeapAllocator tlsf(1, size);

  auto writeTrash = [](void* ptr, size_t size) {
    memset(ptr, 254, size);
  };

  for (int i = 0; i < 3; ++i) {
    auto block = tlsf.allocate(14912);
    writeTrash(block, 14912);
    REQUIRE(block);
    auto block2 = tlsf.allocate(7240);
    writeTrash(block2, 7240);
    REQUIRE(block2);
    {
      std::vector<void*> ptrs;
      for (int k = 0; k < 4; ++k)
      {
        auto ptr = tlsf.allocate(2168);
        writeTrash(ptr, 2168);
        ptrs.push_back(ptr);
      }
      for (int k = 0; k < 4; ++k)
        tlsf.deallocate(ptrs[k]);
    }
    auto block3 = tlsf.allocate(3512);
    writeTrash(block3, 3512);
    REQUIRE(block3);
    auto block4 = tlsf.allocate(104);
    writeTrash(block4, 104);
    REQUIRE(block4);
    auto block5 = tlsf.allocate(696);
    writeTrash(block5, 696);
    REQUIRE(block5);
    tlsf.deallocate(block5);
    auto block6 = tlsf.allocate(696);
    writeTrash(block6, 696);
    REQUIRE(block6);
    auto block7 = tlsf.allocate(368);
    writeTrash(block7, 368);
    REQUIRE(block7);
    auto block8 = tlsf.allocate(696);
    writeTrash(block8, 696);
    REQUIRE(block8);
    auto block9 = tlsf.allocate(128);
    writeTrash(block9, 128);
    REQUIRE(block9);
    tlsf.deallocate(block9);
    tlsf.deallocate(block8);
    tlsf.deallocate(block7);
    tlsf.deallocate(block6);
    tlsf.deallocate(block4);
    tlsf.deallocate(block3);
    tlsf.deallocate(block2);
    tlsf.deallocate(block);
  }
}

/*
TEST_CASE("failed real world case 1") {
  void* heap = malloc(50331648);
  higanbana::HeapAllocatorRaw tlsf(heap, 50331648, 131072);
  auto block = tlsf.allocate(50200588, 131072);
  REQUIRE(block);
  free(heap);
}*/