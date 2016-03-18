#include "network.hpp"
#include "core/src/math/mat_templated.hpp"
#include "core/src/tools/bentsumaakaa.hpp"

namespace faze
{
	void testNetwork(Logger& log)
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

		Matrix2<double, 1, 2> input { 1.0, 1.0};

		NeuralNetwork<100, 2, 1, 1> ann;
		auto result = ann.forward(input);
		printMat(result);
		Matrix2<double, 1, 1> expected { 2.0};
		printMat(expected);
		auto oo = ann.costFunction(expected);
		F_LOG("costFunction: %f\n", oo);
		//ann.costFunctionPrime(input, expected);

		//auto numGrad = ann.computeNumericalGradient(input, expected);
		//auto grad = ann.computeGradients(input, expected);
		//printMat(ann.computeGradients(input, expected));
		//printMat(ann.computeNumericalGradient(input, expected));

		//printMat(ann.forward(input));
		auto trainNetwork = [&](auto& nn, auto input, auto output)
		{
			F_LOG("TRAINING A NETWORK!!!!!!!!!!!\n");
			//printMat(input);
			//printMat(output);
			//printMat(nn.forward(input));
			auto before = nn.costFunction(output);
			auto iter = nn.train(input, output);
			nn.forward(input);
			auto after = nn.costFunction(output);
			F_LOG("costFunction before: %.6f after: %.6f\n", before, after);
			F_LOG("training took %zu iterations.\n", iter);
			//printMat(nn.computeGradients(input, output));
			//printMat(nn.computeNumericalGradient(input, output));
			//printMat(nn.forward(input));
			log.update();
			return iter;
		};
		uint64_t totalIterations = 0;
		totalIterations += trainNetwork(ann, Matrix2<double, 1, 2>{ 0.0, 1.0}, Matrix2<double, 1, 1>{ 1.0});
		totalIterations += trainNetwork(ann, Matrix2<double, 1, 2>{ 1.0, 0.0}, Matrix2<double, 1, 1>{ 1.0});

		F_LOG("Overall %zu iterations to train the network.\n", totalIterations);

		printMat(ann.forward(Matrix2<double, 1, 2>{ 5, 12}));
		printMat(ann.forward(Matrix2<double, 1, 2>{ 100, 50}));
		printMat(ann.forward(Matrix2<double, 1, 2>{ 1, 2}));
		printMat(ann.forward(Matrix2<double, 1, 2>{ 123456, 234567}));
		// close perfect answers above.

		// this doesnt really work.
		NeuralNetwork<10, 2, 1, 1> ann2;
		totalIterations = 0;
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 1.0, 0.0}, Matrix2<double, 1, 1>{0.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 1.0, 1.0}, Matrix2<double, 1, 1>{1.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 2.0, 4.0}, Matrix2<double, 1, 1>{8.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 0.0, 1.0}, Matrix2<double, 1, 1>{0.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 4.0, 2.0}, Matrix2<double, 1, 1>{8.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 1.0, 0.0}, Matrix2<double, 1, 1>{0.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 1.0, 1.0}, Matrix2<double, 1, 1>{1.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 2.0, 4.0}, Matrix2<double, 1, 1>{8.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 0.0, 1.0}, Matrix2<double, 1, 1>{0.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 4.0, 2.0}, Matrix2<double, 1, 1>{8.0});
		totalIterations += trainNetwork(ann2, Matrix2<double, 1, 2>{ 4.0, 4.0}, Matrix2<double, 1, 1>{16.0});

		printMat(ann2.forward(Matrix2<double, 1, 2>{ 4.0, 1.0}));
		printMat(ann2.forward(Matrix2<double, 1, 2>{ 9.0, 2.0}));
		printMat(ann2.forward(Matrix2<double, 1, 2>{ 9.0, 9.0}));

		auto val = b.stop();

		F_LOG("matrices took %f milliseconds\n", static_cast<float>(val) / 1000000.f);
	}

}
