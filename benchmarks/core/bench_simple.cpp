#include <catch2/catch_all.hpp>
#include <map>

namespace {
    std::uint64_t Fibonacci(std::uint64_t number) {
        return number < 2 ? 1 : Fibonacci(number - 1) + Fibonacci(number - 2);
    }
}


TEST_CASE("Benchmark Fibonacci", "[benchmark]") {
    CHECK(Fibonacci(0) == 1);
    // some more asserts..
    CHECK(Fibonacci(5) == 8);
    // some more asserts..

    BENCHMARK("Fibonacci 20") {
        return Fibonacci(20);
    };

    BENCHMARK("Fibonacci 25") {
        return Fibonacci(25);
    };

    BENCHMARK("Fibonacci 30") {
        return Fibonacci(30);
    };

    BENCHMARK("Fibonacci 35") {
        return Fibonacci(35);
    };
}

TEST_CASE("Benchmark containers", "[benchmark]") {
    static const int size = 100;

    std::vector<int> v;
    std::map<int, int> m;

    SECTION("without generator") {
        BENCHMARK("Load up a vector") {
            v = std::vector<int>();
            for (int i = 0; i < size; ++i)
                v.push_back(i);
        };
        REQUIRE(v.size() == size);

        // test optimizer control
        BENCHMARK("Add up a vector's content") {
            uint64_t add = 0;
            for (int i = 0; i < size; ++i)
                add += v[i];
            return add;
        };

        BENCHMARK("Load up a map") {
            m = std::map<int, int>();
            for (int i = 0; i < size; ++i)
                m.insert({ i, i + 1 });
        };
        REQUIRE(m.size() == size);

        BENCHMARK("Reserved vector") {
            v = std::vector<int>();
            v.reserve(size);
            for (int i = 0; i < size; ++i)
                v.push_back(i);
        };
        REQUIRE(v.size() == size);

        BENCHMARK("Resized vector") {
            v = std::vector<int>();
            v.resize(size);
            for (int i = 0; i < size; ++i)
                v[i] = i;
        };
        REQUIRE(v.size() == size);

        int array[size];
        BENCHMARK("A fixed size array that should require no allocations") {
            for (int i = 0; i < size; ++i)
                array[i] = i;
        };
        int sum = 0;
        for (int i = 0; i < size; ++i)
            sum += array[i];
        REQUIRE(sum > size);

        SECTION("XYZ") {

            BENCHMARK_ADVANCED("Load up vector with chronometer")(Catch::Benchmark::Chronometer meter) {
                std::vector<int> k;
                meter.measure([&](int idx) {
                    k = std::vector<int>();
                    for (int i = 0; i < size; ++i)
                        k.push_back(idx);
                });
                REQUIRE(k.size() == size);
            };

            int runs = 0;
            BENCHMARK("Fill vector indexed", benchmarkIndex) {
                v = std::vector<int>();
                v.resize(size);
                for (int i = 0; i < size; ++i)
                    v[i] = benchmarkIndex;
                runs = benchmarkIndex;
            };

            for (size_t i = 0; i < v.size(); ++i) {
                REQUIRE(v[i] == runs);
            }
        }
    }

    SECTION("with generator") {
        auto generated = GENERATE(range(0, 10));
        BENCHMARK("Fill vector generated") {
            v = std::vector<int>();
            v.resize(size);
            for (int i = 0; i < size; ++i)
                v[i] = generated;
        };
        for (size_t i = 0; i < v.size(); ++i) {
            REQUIRE(v[i] == generated);
        }
    }
}