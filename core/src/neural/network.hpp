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
	  Matrix<double, sampleCount, hiddenSize> m_z2a2;
	  Matrix<double, hiddenSize, outputSize> m_w2;
	  Matrix<double, sampleCount, outputSize> m_z3yHat;

	  inline double sigmoid(double& val)
	  {
		  return 1.0 / (1.0 + exp(-val));
	  }
  public:

	  NeuralNetwork()
	  {
		  std::random_device rd;
		  std::mt19937 mt(rd());
		  std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

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
		  m_z2a2 = input * m_w1;
		  for (int i = 0; i < sampleCount; ++i)
		  {
			  for (int k = 0; k < hiddenSize; ++k)
			  {
				  m_z2a2[i][k] = sigmoid(m_z2a2[i][k]);
			  }
		  }
		  m_z3yHat = m_z2a2 * m_w2;
		  for (int i = 0; i < sampleCount; ++i)
		  {
			  for (int k = 0; k < outputSize; ++k)
			  {
				  m_z3yHat[i][k] = sigmoid(m_z3yHat[i][k]);
			  }
		  }
		  return m_z3yHat;
	  }
  };
}
