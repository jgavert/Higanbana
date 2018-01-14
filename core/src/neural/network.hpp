#pragma once
#include "core/src/math/math.hpp"
#include "core/src/system/logger.hpp"
#include <cmath>
#include <random>

namespace faze
{
  void testNetwork(Logger& log);

  namespace math
  {

#define printMat(mat) F_LOG("%s\n", faze::math::toString(mat).c_str())


    template<int hiddenSize, int inputSize, int outputSize, int sampleCount>
    class NeuralNetwork
    {
    private:
      Matrix<inputSize, hiddenSize, double> m_w1;
      Matrix<sampleCount, hiddenSize, double> m_z2;
      Matrix<sampleCount, hiddenSize, double> m_a2;
      Matrix<hiddenSize, outputSize, double> m_w2;
      Matrix<sampleCount, outputSize, double> m_z3;
      Matrix<sampleCount, outputSize, double> m_yHat;

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

      inline double sigmoidPrime(double) // f pilkku
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

      math::Matrix<sampleCount, outputSize, double> forward(math::Matrix<sampleCount, inputSize, double> input)
      {
        m_z2 = mul(input, m_w1);
        m_a2 = transform(m_z2, [this](double x) {return sigmoid(x); });
        m_z3 = mul(m_a2, m_w2);
        m_yHat = transform(m_z3, [this](double x) {return sigmoid(x); });
        return m_yHat;
      }


      double costFunction(decltype(m_z3)& expectedOutput)
      {
        return 0.5 * sum(transform(sub(expectedOutput, m_yHat), [this](double x) {return pow(x, 2.0); }));
      }

      void costFunctionPrime(Matrix<sampleCount, inputSize, double>& input, decltype(m_z3)& expectedOutput)
      {
        auto delta3 = mulElementWise(mul(sub(expectedOutput, m_yHat), { -1.0 }), transform(m_z3, [this](double x) {return sigmoidPrime(x); }));
        auto dJdW2 = mul(transpose(m_a2), delta3);
        printMat(dJdW2);
        auto delta2 = mul(mul(delta3, transpose(m_w2)), transform(m_z2, [this](double x) {return sigmoidPrime(x); }));
        auto dJdW1 = mul(transpose(input), delta2);
        printMat(dJdW1);
      }

      auto computeGradients(Matrix<sampleCount, inputSize, double>& input, decltype(m_z3)& expectedOutput)
      {
        printMat(m_w1);
        printMat(m_w2);
        auto delta3 = mulElementWise(mul(sub(expectedOutput, m_yHat), { -1.0 }), transform(m_z3, [this](double x) {return sigmoidPrime(x); }));
        auto dJdW2 = mul(transpose(m_a2), delta3);
        //printMat(dJdW2);
        auto delta2 = mulElementWise(mul(delta3, transpose(m_w2)), transform(m_z2, [this](double x) {return sigmoidPrime(x); }));
        auto dJdW1 = mul(transpose(input), delta2);
        //printMat(dJdW1);

        return concatenateToSingleDimension(dJdW1, dJdW2);
      }

      // to verify our gradients from above
      auto computeNumericalGradient(Matrix<sampleCount, inputSize, double>& input, decltype(m_z3)& expectedOutput)
      {
        using namespace math;
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
          numgrad(i) = (loss2 - loss1) / (2.0 * e);
          perturb(i) = 0;

        }
        extractMatrices(paramsInitial, m_w1, m_w2);
        return numgrad;
      }

      uint64_t train(Matrix<sampleCount, inputSize, double>& input, decltype(m_z3)& expectedOutput)
      {
        double e = 0.00001;
        double targetError = 0.1;
        double learningSpeed = 10;
        forward(input);
        double prevCost = costFunction(expectedOutput);
        double sanity = prevCost;
        double checkProgress = 0.1;
        double underError = prevCost * checkProgress;
        uint64_t iter = 0;
        bool doOnce = false;

        auto learn = [&](double& currentValue)
        {
          currentValue += e;
          forward(input);
          currentValue -= e;
          double currentCost = costFunction(expectedOutput);
          double rawLearning = (prevCost - currentCost) / e;
          double localLearningSpeed = learningSpeed;
          currentValue += localLearningSpeed * rawLearning;

          iter++;
          forward(input);
          currentCost = costFunction(expectedOutput);
          // check if we learned too fast... and reduce learning speed until its fine
          while (currentCost > prevCost && localLearningSpeed > 0.0001)
          {
            // revert changes
            currentValue -= localLearningSpeed * rawLearning;
            // change multiplier
            localLearningSpeed *= 0.05;
            // apply changes
            currentValue += localLearningSpeed * rawLearning;
            // calc new error
            forward(input);
            currentCost = costFunction(expectedOutput);
            iter++;
          }
          if (currentCost > prevCost)
          {
            currentValue -= localLearningSpeed * rawLearning;
            forward(input);
            currentCost = costFunction(expectedOutput);
          }
          prevCost = currentCost;
        };

        while (doOnce || (prevCost > targetError && iter < 1000000))
        {
          doOnce = false;
          for (int i = 0; i < static_cast<int>(m_w1.size()); ++i)
          {
            learn(m_w1(i));
          }
          for (int i = 0; i < static_cast<int>(m_w2.size()); ++i)
          {
            learn(m_w2(i));
          }
          if (prevCost >= sanity)
          {
            //F_LOG("found a upward slope: iter %zu current err: %.10f\n", iter, prevCost);
            return iter;
          }
          sanity = prevCost;
          if (prevCost < underError)
          {
            underError = prevCost * checkProgress;
            //F_LOG("got under new margin at iter %zu current err: %.10f\n", iter, prevCost);
          }
        }
        return iter;
      }
    };
  }
}
