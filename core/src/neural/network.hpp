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

    /*
	  inline double sigmoid(double val) // f
	  {
		  return 1.0 / (1.0 + exp(-val));
	  }

	  inline double sigmoidPrime(double val) // f pilkku
	  {
		  return exp(-val) / pow(1.0 + exp(-val), 2.0);
	  }*/

    inline double sigmoid(double val) // f
    {
      return val;
    }

    inline double sigmoidPrime(double ) // f pilkku
    {
      return 1;
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

	  Matrix2<double, sampleCount, outputSize> forward(Matrix2<double, sampleCount, inputSize>& input)
	  {
		  using namespace MatrixMath;
		  m_z2 = mul(input, m_w1);
		  m_a2 = transform(m_z2, [this](double x) {return sigmoid(x); });
		  m_z3 = mul(m_a2, m_w2);
		  auto yhat = transform(m_z3, [this](double x) {return sigmoid(x); });
		  return yhat;
	  }


	  double costFunction(Matrix2<double, sampleCount, inputSize>& input, decltype(m_z3)& expectedOutput)
	  {
		  using namespace MatrixMath;
		  auto yHat = forward(input);
		  return 0.5 * sum(transform(sub(expectedOutput, yHat), [this](double x) {return pow(x, 2.0); }));
	  }

	  void costFunctionPrime(Matrix2<double, sampleCount, inputSize>& input, decltype(m_z3)& expectedOutput)
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

	  auto computeGradients(Matrix2<double, sampleCount, inputSize>& input, decltype(m_z3)& expectedOutput)
	  {
		  using namespace MatrixMath;
		  auto yHat = forward(input);

      printMat(m_w1);
      printMat(m_w2);
		  auto delta3 = multiplyElementWise(mulScalar(sub(expectedOutput, yHat), { -1.0 }), transform(m_z3, [this](double x) {return sigmoidPrime(x); }));
		  auto dJdW2 = mul(transpose(m_a2), delta3);
		  //printMat(dJdW2);
		  auto delta2 = multiplyElementWise(mul(delta3, transpose(m_w2)), transform(m_z2, [this](double x) {return sigmoidPrime(x); }));
		  auto dJdW1 = mul(transpose(input), delta2);
		  //printMat(dJdW1);

		  return concatenateToSingleDimension(dJdW1, dJdW2);
	  }
	  
    // to verify our gradients from above
	  auto computeNumericalGradient(Matrix2<double, sampleCount, inputSize>& input, decltype(m_z3)& expectedOutput)
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

    void train(Matrix2<double, sampleCount, inputSize>& input, decltype(m_z3)& expectedOutput)
    {
      using namespace MatrixMath;
      double e = 0.0001;
      double targetError = 0.01;
      double prevCost = costFunction(input, expectedOutput);
      double sanity = prevCost;
      double checkProgress = 0.1;
      double underError = prevCost*checkProgress;
      uint64_t iter = 0;
      while (prevCost > targetError && iter < 1000000)
      {
        for (int i = 0; i < m_w1.data.size(); ++i)
        {
          m_w1(i) += e;
          double currentCost = costFunction(input, expectedOutput);
          m_w1(i) -= e;

          double apua = 0.1*(prevCost - currentCost);
          m_w1(i) += apua;

          prevCost = costFunction(input, expectedOutput);
          //F_LOG("pc: %.17f cc: %.17f == %.17f\n", prevCost, currentCost, apua);

        }
        for (int i = 0; i < m_w2.data.size(); ++i)
        {
          m_w2(i) += e;
          double currentCost = costFunction(input, expectedOutput);
          m_w2(i) -= e;

          double apua = 0.1*(prevCost - currentCost);
          m_w2(i) += apua;
          
          prevCost = costFunction(input, expectedOutput);
          //F_LOG("pc: %f cc: %f == %f\n", prevCost, currentCost, apua);
        }
        //prevCost = costFunction(input, expectedOutput);
        iter++;
        if (prevCost >= sanity)
        {
          F_LOG("found a upward slope: iter %zu current err: %.10f\n", iter, prevCost);
          return;
        }
        sanity = prevCost;
        if (prevCost < underError)
        {
          underError = prevCost*checkProgress;
          F_LOG("got under new margin at iter %zu current err: %.10f\n", iter, prevCost);
        }
      }
    }
  };
}
