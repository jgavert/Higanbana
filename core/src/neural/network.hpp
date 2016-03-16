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
	  Matrix<double, inputSize, hiddenSize> m_w1;
	  Matrix<double, sampleCount, hiddenSize> m_z2;
	  Matrix<double, sampleCount, hiddenSize> m_a2;
	  Matrix<double, hiddenSize, outputSize> m_w2;
	  Matrix<double, sampleCount, outputSize> m_z3;

	  inline double sigmoid(double val) // f
	  {
		  return 1.0 / (1.0 + exp(-val));
	  }

    inline double sigmoidPrime(double val) // f pilkku
    {
      return exp(-val)/pow(1.0+exp(-val), 2.0);
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
				  m_w1[x][y] = dist(mt);
			  }
		  }
		  for (int y = 0; y < outputSize; ++y)
		  {
			  for (int x = 0; x < hiddenSize; ++x)
			  {
				  m_w2[x][y] = dist(mt);
			  }
		  }
	  }

	  Matrix<double, sampleCount, outputSize> forward(Matrix<double, sampleCount, inputSize> input)
	  {
      using namespace MatrixMath;
      mul(input, m_w1, m_z2);
      transform(m_z2, m_a2, [this](double x) {return sigmoid(x); });
      mul(m_a2, m_w2, m_z3);
      auto yhat = transform(m_z3, [this](double x) {return sigmoid(x); });
      return yhat;
	  }


    double costFunction(Matrix<double, sampleCount, inputSize> input, decltype(m_z3) expectedOutput)
    {
      using namespace MatrixMath;
      auto yHat = forward(input);
      return 0.5*sum(transform((expectedOutput - yHat), [this](double x) {return pow(x, 2.0); }));
    }

    void costFunctionPrime(Matrix<double, sampleCount, inputSize> input, decltype(m_z3) expectedOutput)
    {
      using namespace MatrixMath;
      auto yHat = forward(input);

      auto delta3 = multiplyElementWise((expectedOutput - yHat)*(-1.0), transform(m_z3, [this](double x) {return sigmoidPrime(x); }));
      auto dJdW2 = mul(transpose(m_a2), delta3);

      auto delta2 = mul(mul(delta3, transpose(m_w2)), transform(m_z2, [this](double x) {return sigmoidPrime(x); }));
      auto dJdW1 = mul(transpose(input), delta2);
    }

  };
}
