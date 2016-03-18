#include "network.hpp"
#include "core/src/math/mat_templated.hpp"
#include "core/src/tools/bentsumaakaa.hpp"

namespace faze
{
	void testNetwork()
	{
		Bentsumaakaa b;
		b.start();
		constexpr static int major = 3;
		constexpr static int minor = 2;
		Matrix2<double, major, minor> test;
		Matrix2<double, minor, major> test2;
		for (int i = 0; i < major; ++i)
		{
			for (int k = 0; k < minor; ++k)
			{
				test(i,k) = double(i + k);
			}
		}
		for (int i = 0; i < major; ++i)
		{
			for (int k = 0; k < minor; ++k)
			{
				test2(k,i) = double(i + k);
			}
		}
		auto res = MatrixMath::mul(test, test2);

		auto vect = MatrixMath::concatenateToSingleDimension(test, test2);
		for (int i = 0; i < vect.data.size(); ++i)
		{
			vect(i) = double(i);
		}
		MatrixMath::extractMatrices(vect, test, test2);

		Matrix2<double, 4, 2> input { 1.0, 1.0,\
                                  2.0, 4.0,\
                                  6.0, 1.0,\
                                  1.0, 5.0};

		NeuralNetwork<10, 2, 1, 4> ann;
		auto result = ann.forward(input);
		printMat(result);
		Matrix2<double, 4, 1> expected { 2.0, 6.0, 7.0, 6.0 };
		printMat(expected);
		auto oo = ann.costFunction(input, expected);
		F_LOG("costFunction: %f\n", oo);
		//ann.costFunctionPrime(input, expected);

		//auto numGrad = ann.computeNumericalGradient(input, expected);
		//auto grad = ann.computeGradients(input, expected);
		printMat(ann.computeGradients(input, expected));
		printMat(ann.computeNumericalGradient(input, expected));

    printMat(ann.forward(input));
    ann.train(input, expected);
		printMat(ann.computeGradients(input, expected));
		printMat(ann.computeNumericalGradient(input, expected));
    printMat(ann.forward(input));

		auto val = b.stop();

		F_LOG("matrices took %f milliseconds\n", static_cast<float>(val) / 1000000.f);
	}

}
