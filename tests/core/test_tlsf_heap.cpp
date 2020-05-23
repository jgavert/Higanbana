#include <catch2/catch.hpp>
#include <higanbana/core/platform/definitions.hpp>
#include <higanbana/core/allocators/heap_allocator_raw.hpp>

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

/*
TEST_CASE("failed real world case 1") {
  void* heap = malloc(50331648);
  higanbana::HeapAllocatorRaw tlsf(heap, 50331648, 131072);
  auto block = tlsf.allocate(50200588, 131072);
  REQUIRE(block);
  free(heap);
}*/