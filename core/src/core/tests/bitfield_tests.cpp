#include "core/tests/bitfield_tests.hpp"
#include "core/tests/TestWorks.hpp"
using namespace faze;
bool BitfieldTests::Run()
{
	TestWorks t("BitfieldTests");

	t.addTest("1. empty", []()
	{
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty(0);
		return empty == 0;
	});
	t.addTest("2. empty", []()
	{
    Bitfield<1> b;
    b.setIdxBit(0);
    auto empty = b.skip_find_firstEmpty(0);
		return empty == 1;
	});

  t.addTest("first 64 in order", []()
  {
    Bitfield<1> b;
    for (int i = 0; i < 64; ++i)
    {
      auto empty = b.skip_find_firstEmpty(0);
      if (empty != i)
      {
        return false;
      }
      b.setIdxBit(empty);
    }
    return true;
  });

  t.addTest("first 128 in order", []()
  {
    Bitfield<1> b;
    for (int i = 0; i < 128; ++i)
    {
      auto empty = b.skip_find_firstEmpty(0);
      if (empty != i)
      {
        return false;
      }
      b.setIdxBit(empty);
    }
    return true;
  });

  t.addTest("first block full", []()
  {
    Bitfield<1> b;
    for (int i = 0; i < 128; ++i)
    {
      b.setIdxBit(i);
    }
    auto empty = b.skip_find_firstEmpty(0);
    return  (empty == -1);
  });

  t.addTest("find first from holes", []()
  {
    Bitfield<1> b;
    b.setIdxBit(2);
    auto empty = b.skip_find_firstEmpty(0);
    return  (empty == 0);
  });

  t.addTest("find first from holes (2)", []()
  {
    Bitfield<1> b;
    b.setIdxBit(1);
    b.setIdxBit(2);
    auto empty = b.skip_find_firstEmpty(0);
    return  (empty == 0);
  });

  t.addTest("find first from holes (3)", []()
  {
    Bitfield<1> b;
    for (int i = 0; i <64; ++i)
    {
      b.setIdxBit(i);
    }
    b.setIdxBit(65);
    auto empty = b.skip_find_firstEmpty(0);
    return  (empty == 64);
  });

  t.addTest("find first from holes (4)", []()
  {
    Bitfield<1> b;
    b.setIdxBit(0);
    b.setIdxBit(2);
    auto empty = b.skip_find_firstEmpty(0);
    return  (empty == 1);
  });

  t.addTest("find first from holes (5)", []()
  {
    Bitfield<1> b;
    for (int i = 0; i <127; ++i)
    {
      b.setIdxBit(i);
    }
    auto empty = b.skip_find_firstEmpty(0);
    return  (empty == 127);
  });

  t.addTest("find first with offset 32", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_offset(0, 32);
    return  (empty == 32);
  });

  t.addTest("find first with offset 17", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_offset(0, 17);
    return  (empty == 17);
  });
  
  t.addTest("find first with offset 64", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_offset(0, 64);
    return  (empty == 64);
  });

  t.addTest("find first with offset 65", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_offset(0, 65);
    return  (empty == 65);
  });

  t.addTest("find first with offset 127", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_offset(0, 127);
    return  (empty == 127);
  });

  t.addTest("find first with offset 128", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_offset(0, 128);
    return  (empty == -1);
  });

  t.addTest("1. empty (full version)", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_full();
    return empty == 0;
  });
  t.addTest("2. empty (full version)", []()
  {
    Bitfield<1> b;
    b.setIdxBit(0);
    auto empty = b.skip_find_firstEmpty_full();
    return empty == 1;
  });

  t.addTest("first 64 in order (full version)", []()
  {
    Bitfield<1> b;
    for (int i = 0; i < 64; ++i)
    {
      auto empty = b.skip_find_firstEmpty_full();
      if (empty != i)
      {
        return false;
      }
      b.setIdxBit(empty);
    }
    return true;
  });

  t.addTest("first 128 in order (full version)", []()
  {
    Bitfield<1> b;
    for (int i = 0; i < 128; ++i)
    {
      auto empty = b.skip_find_firstEmpty_full();
      if (empty != i)
      {
        return false;
      }
      b.setIdxBit(empty);
    }
    return true;
  });

  t.addTest("first 256 in order (full version)", []()
  {
    Bitfield<2> b;
    for (int i = 0; i < 256; ++i)
    {
      auto empty = b.skip_find_firstEmpty_full();
      if (empty != i)
      {
        return false;
      }
      b.setIdxBit(empty);
    }
    return true;
  });

  t.addTest("first block full 128 (full version)", []()
  {
    Bitfield<1> b;
    for (int i = 0; i < 128; ++i)
    {
      b.setIdxBit(i);
    }
    auto empty = b.skip_find_firstEmpty_full();
    return  (empty == -1);
  });

  t.addTest("first block full 256 (full version)", []()
  {
    Bitfield<2> b;
    for (int i = 0; i < 256; ++i)
    {
      b.setIdxBit(i);
    }
    auto empty = b.skip_find_firstEmpty_full();
    return  (empty == -1);
  });

  t.addTest("find first from holes  (full version)", []()
  {
    Bitfield<1> b;
    b.setIdxBit(2);
    auto empty = b.skip_find_firstEmpty_full();
    return  (empty == 0);
  });

  t.addTest("find first from holes (2) (full version)", []()
  {
    Bitfield<1> b;
    b.setIdxBit(1);
    b.setIdxBit(2);
    auto empty = b.skip_find_firstEmpty_full();
    return  (empty == 0);
  });

  t.addTest("find first from holes (3) (full version)", []()
  {
    Bitfield<1> b;
    for (int i = 0; i <64; ++i)
    {
      b.setIdxBit(i);
    }
    b.setIdxBit(65);
    auto empty = b.skip_find_firstEmpty_full();
    return  (empty == 64);
  });

  t.addTest("find first from holes (4) (full version)", []()
  {
    Bitfield<1> b;
    b.setIdxBit(0);
    b.setIdxBit(2);
    auto empty = b.skip_find_firstEmpty_full();
    return  (empty == 1);
  });

  t.addTest("find first from holes (5) (full version)", []()
  {
    Bitfield<1> b;
    for (int i = 0; i <127; ++i)
    {
      b.setIdxBit(i);
    }
    auto empty = b.skip_find_firstEmpty_full();
    return  (empty == 127);
  });

  t.addTest("find first from holes (6) (full version)", []()
  {
    Bitfield<2> b;
    for (int i = 0; i <200; ++i)
    {
      b.setIdxBit(i);
    }
    b.setIdxBit(201);
    auto empty = b.skip_find_firstEmpty_full();
    return  (empty == 200);
  });

  t.addTest("find first with offset 32 (full version)", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_full_offset(32);
    return  (empty == 32);
  });

  t.addTest("find first with offset 17 (full version)", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_full_offset(17);
    return  (empty == 17);
  });

  t.addTest("find first with offset 64 (full version)", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_full_offset(64);
    return  (empty == 64);
  });

  t.addTest("find first with offset 65 (full version)", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_full_offset(65);
    return  (empty == 65);
  });

  t.addTest("find first with offset 127 (full version)", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_full_offset(127);
    return  (empty == 127);
  });

  t.addTest("find first with offset 128 (full version)", []()
  {
    Bitfield<1> b;
    auto empty = b.skip_find_firstEmpty_full_offset(128);
    return  (empty == -1);
  });

  t.addTest("find first with offset 200 (full version)", []()
  {
    Bitfield<2> b;
    auto empty = b.skip_find_firstEmpty_full_offset(200);
    return  (empty == 200);
  });

  t.addTest("find first with offset 256 (full version)", []()
  {
    Bitfield<2> b;
    auto empty = b.skip_find_firstEmpty_full_offset(256);
    return  (empty == -1);
  });

	return t.runTests();
}