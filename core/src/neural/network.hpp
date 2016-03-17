#pragma once
#include "core/src/math/mat_templated.hpp"
#include <cmath>
#include <random>

namespace faze
{
  void testNetwork();

  template<int hiddenSize, int inputSize, int outputSize, int sampleCount>
  class NeuralNetwork
  {
  private:
	  Matrix2<double, inputSize, hiddenSize> m_w1;
	  Matrix2<double, sampleCount, hiddenSize> m_z2;
	  Matrix2<double, sampleCount, hiddenSize> m_a2;
	  Matrix2<double, hiddenSize, outputSize> m_w2;
	  Matrix2<double, sampleCount, outputSize> m_z3;

	  inline double sigmoid(double val) // f
	  {
		  return 1.0 / (1.0 + exp(-val));
	  }

	  inline double sigmoidPrime(double val) // f pilkku
	  {
		  return exp(-val) / pow(1.0 + exp(-val), 2.0);
	  }

  public:

	  NeuralNetwork()
	  {
		  std::random_device rd;
		  std::mt19937 mt(rd());
		  std::uniform_real_distribution<double> dist(0.0, 1.0);

		  for (int y = 0; y < hiddenSize; ++y)
		  {
			  for (int x = 0; x < inputSize; ++x)
			  {
				  m_w1(x, y) = dist(mt);
			  }
		  }
		  for (int y = 0; y < outputSize; ++y)
		  {
			  for (int x = 0; x < hiddenSize; ++x)
			  {
				  m_w2(x, y) = dist(mt);
			  }
		  }
	  }

	  Matrix2<double, sampleCount, outputSize> forward(Matrix2<double, sampleCount, inputSize> input)
	  {
		  using namespace MatrixMath;
		  m_z2 = mul(input, m_w1);
		  m_a2 = transform(m_z2, [this](double x) {return sigmoid(x); });
		  m_z3 = mul(m_a2, m_w2);
		  auto yhat = transform(m_z3, [this](double x) {return sigmoid(x); });
		  return yhat;
	  }


	  double costFunction(Matrix2<double, sampleCount, inputSize> input, decltype(m_z3) expectedOutput)
	  {
		  using namespace MatrixMath;
		  auto yHat = forward(input);
		  return 0.5 * sum(transform(sub(expectedOutput, yHat), [this](double x) {return pow(x, 2.0); }));
	  }

	  void costFunctionPrime(Matrix2<double, sampleCount, inputSize> input, decltype(m_z3) expectedOutput)
	  {
		  using namespace MatrixMath;
		  auto yHat = forward(input);

		  auto delta3 = multiplyElementWise(mulScalar(sub(expectedOutput, yHat), { -1.0 }), transform(m_z3, [this](double x) {return sigmoidPrime(x); }));
		  auto dJdW2 = mul(transpose(m_a2), delta3);
		  printMat(dJdW2);
		  auto delta2 = mul(mul(delta3, transpose(m_w2)), transform(m_z2, [this](double x) {return sigmoidPrime(x); }));
		  auto dJdW1 = mul(transpose(input), delta2);
		  printMat(dJdW1);
	  }

	  auto computeGradients(Matrix2<double, sampleCount, inputSize> input, decltype(m_z3) expectedOutput)
	  {
		  using namespace MatrixMath;
		  auto yHat = forward(input);

		  auto delta3 = multiplyElementWise(mulScalar(sub(expectedOutput, yHat), { -1.0 }), transform(m_z3, [this](double x) {return sigmoidPrime(x); }));
		  auto dJdW2 = mul(transpose(m_a2), delta3);
		  //printMat(dJdW2);
		  auto delta2 = multiplyElementWise(mul(delta3, transpose(m_w2)), transform(m_z2, [this](double x) {return sigmoidPrime(x); }));
		  auto dJdW1 = mul(transpose(input), delta2);
		  //printMat(dJdW1);

		  return concatenateToSingleDimension(dJdW1, dJdW2);
	  }
	  
	  auto computeNumericalGradient(Matrix2<double, sampleCount, inputSize> input, decltype(m_z3) expectedOutput)
	  {
		  using namespace MatrixMath;
		  auto paramsInitial = concatenateToSingleDimension(m_w1, m_w2);
		  decltype(paramsInitial) numgrad{};
		  decltype(paramsInitial) perturb{};
		  double e = 0.00001;

		  for (int i = 0; i < paramsInitial.data.size(); ++i)
		  {
			  // Set perturbation vector
			  
			  perturb(i) = e;
			  extractMatrices(add(paramsInitial, perturb), m_w1, m_w2);
			  auto loss2 = costFunction(input, expectedOutput);

			  extractMatrices(sub(paramsInitial, perturb), m_w1, m_w2);
			  auto loss1 = costFunction(input, expectedOutput);
			  // compute numerical gradient
			  numgrad(i) = (loss2 - loss1)/ (2.0 * e);
			  perturb(i) = 0;
			  
		  }
		  extractMatrices(paramsInitial, m_w1, m_w2);
		  return numgrad;
	  }
  };
}
