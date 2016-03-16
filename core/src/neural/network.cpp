#include "network.hpp"
#include "core/src/math/mat_templated.hpp"

namespace faze
{
	void testNetwork()
	{
		constexpr static int major = 3;
		constexpr static int minor = 2;
		Matrix<double, major, minor> test;
		Matrix<double, minor, major> test2;
		for (int i = 0; i < major; ++i)
		{
			for (int k = 0; k < minor; ++k)
			{
				test[i][k] = double(i + k);
			}
		}
		for (int i = 0; i < major; ++i)
		{
			for (int k = 0; k < minor; ++k)
			{
				test2[k][i] = double(i + k);
			}
		}
		auto res{ test * test2 };

		
	}

}
