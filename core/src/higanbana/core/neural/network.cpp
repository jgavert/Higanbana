#include "higanbana/core/neural/network.hpp"
#include "higanbana/core/math/math.hpp"
#include "higanbana/core/tools/bentsumaakaa.hpp"



namespace higanbana
{
  using namespace math;
	void testNetwork(Logger& log)
	{
		Bentsumaakaa b;
		b.start();
		constexpr static int major = 3;
		constexpr static int minor = 2;
		Matrix<major, minor, double> test;
		Matrix<minor, major, double> test2;
		for (int i = 0; i < major; ++i)
		{
			for (int k = 0; k < minor; ++k)
			{
				test(k,i) = double(i + k);
			}
		}
		for (int i = 0; i < major; ++i)
		{
			for (int k = 0; k < minor; ++k)
			{
				test2(i,k) = double(i + k);
			}
		}
		mul(test, test2);

		auto vect = concatenateToSingleDimension(test, test2);
		for (int i = 0; i < static_cast<int>(vect.size()); ++i)
		{
			vect(i) = double(i);
		}
		extractMatrices(vect, test, test2);

		Matrix<1, 2, double> input { 1.0, 1.0};

		NeuralNetwork<100, 2, 1, 1> ann;
		auto result = ann.forward(input);
    //HIGAN_LOG("%s\n", toString(result).c_str());
		Matrix<1, 1, double> expected { 2.0};
    //HIGAN_LOG("%s\n", toString(expected).c_str());
		auto oo = ann.costFunction(expected);
		HIGAN_LOG("costFunction: %f\n", oo);
		//ann.costFunctionPrime(input, expected);

		//auto numGrad = ann.computeNumericalGradient(input, expected);
		//auto grad = ann.computeGradients(input, expected);
		//printMat(ann.computeGradients(input, expected));
		//printMat(ann.computeNumericalGradient(input, expected));

		//printMat(ann.forward(input));
		auto trainNetwork = [&](auto& nn, auto input, auto output)
		{
			HIGAN_LOG("TRAINING A NETWORK!!!!!!!!!!!\n");
			//printMat(input);
			//printMat(output);
			//printMat(nn.forward(input));
			auto before = nn.costFunction(output);
			auto iter = nn.train(input, output);
			nn.forward(input);
			auto after = nn.costFunction(output);
			HIGAN_LOG("costFunction before: %.6f after: %.6f\n", before, after);
			HIGAN_LOG("training took %zu iterations.\n", iter);
			//printMat(nn.computeGradients(input, output));
			//printMat(nn.computeNumericalGradient(input, output));
			//printMat(nn.forward(input));
			log.update();
			return iter;
		};
		uint64_t totalIterations = 0;
		totalIterations += trainNetwork(ann, Matrix<1, 2, double>{ 0.0, 1.0}, Matrix<1, 1, double>{ 1.0});
		totalIterations += trainNetwork(ann, Matrix<1, 2, double>{ 1.0, 0.0}, Matrix<1, 1, double>{ 1.0});

		HIGAN_LOG("Overall %zu iterations to train the network.\n", totalIterations);

		//printMat(ann.forward(Matrix<1, 2, double>{ 5, 12}));
		//printMat(ann.forward(Matrix<1, 2, double>{ 100, 50}));
		//printMat(ann.forward(Matrix<1, 2, double>{ 1, 2}));
		//printMat(ann.forward(Matrix<1, 2, double>{ 123456, 234567}));
		// close perfect answers above.

		// this doesnt really work.
		NeuralNetwork<10, 2, 1, 1> ann2;
		totalIterations = 0;
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 1.0, 0.0}, Matrix<1, 1, double>{0.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 1.0, 1.0}, Matrix<1, 1, double>{1.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 2.0, 4.0}, Matrix<1, 1, double>{8.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 0.0, 1.0}, Matrix<1, 1, double>{0.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 4.0, 2.0}, Matrix<1, 1, double>{8.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 1.0, 0.0}, Matrix<1, 1, double>{0.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 1.0, 1.0}, Matrix<1, 1, double>{1.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 2.0, 4.0}, Matrix<1, 1, double>{8.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 0.0, 1.0}, Matrix<1, 1, double>{0.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 4.0, 2.0}, Matrix<1, 1, double>{8.0});
		totalIterations += trainNetwork(ann2, Matrix<1, 2, double>{ 4.0, 4.0}, Matrix<1, 1, double>{16.0});

		//printMat(ann2.forward(Matrix<1, 2, double>{ 4.0, 1.0}));
		//printMat(ann2.forward(Matrix<1, 2, double>{ 9.0, 2.0}));
		//printMat(ann2.forward(Matrix<1, 2, double>{ 9.0, 9.0}));

		auto val = b.stop();

		HIGAN_LOG("matrices took %f milliseconds\n", static_cast<float>(val) / 1000000.f);
	}

}
