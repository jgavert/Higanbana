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

		Matrix<double, 3, 2> input;
		input[0] = { 3.0,5.0 };
		input[1] = { 5.0,1.0 };
		input[2] = { 10.0,2.0 };

		NeuralNetwork<3, 2, 1, 3> ann;
		auto res2 = ann.forward(input);

    Matrix<double, 3, 1> expected;
    expected[0] = { 75.0 };
    expected[1] = { 82.0 };
    expected[2] = { 93.0 };

    auto oo = ann.costFunction(input, expected);
    printf("%f\n", oo);
    ann.costFunctionPrime(input, expected);

	}

}
