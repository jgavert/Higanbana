#include "bitfield_tests.hpp"
#include "TestWorks.hpp"
using namespace faze;
bool BitfieldTests::Run()
{
	TestWorks t("BitfieldTests");

	t.addTest("basic", []()
	{
		return false;
	});

	return t.runTests();
}