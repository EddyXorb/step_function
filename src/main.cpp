#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <step_function.hpp>

using namespace std;

struct TestKeyType
{
   unsigned int val;
   constexpr TestKeyType(unsigned int val_) : val(val_) {}
   constexpr TestKeyType(const TestKeyType& other) : val(other.val) {}
   constexpr TestKeyType(TestKeyType&& other) : val(std::move(other.val)) {}
   constexpr bool operator<(const TestKeyType& other) const { return val < other.val; }
   TestKeyType& operator=(const TestKeyType& other)
   {
      val = other.val;
      return *this;
   }
   TestKeyType& operator=(TestKeyType&& other) noexcept
   {
      val = std::move(other.val);
      return *this;
   }
};

struct TestFloatKeyType
{
   double val;
   constexpr TestFloatKeyType(double val_) : val(val_) {}
   constexpr TestFloatKeyType(const TestFloatKeyType& other) : val(other.val) {}
   constexpr TestFloatKeyType(TestFloatKeyType&& other) : val(std::move(other.val)) {}
   constexpr bool operator<(const TestFloatKeyType& other) const { return val < other.val; }
   TestFloatKeyType& operator=(const TestFloatKeyType& other)
   {
      val = other.val;
      return *this;
   }
   TestFloatKeyType& operator=(TestFloatKeyType&& other) noexcept
   {
      val = std::move(other.val);
      return *this;
   }
};

struct TestValueType
{
   char val;
   TestValueType(char c) : val(c) {}
   TestValueType(const TestValueType& other) : val(other.val) {}
   TestValueType(TestValueType&& other) noexcept : val(std::move(other.val)) {}
   bool operator==(const TestValueType& other) const { return val == other.val; }
   TestValueType& operator=(const TestValueType& other)
   {
      val = other.val;
      return *this;
   }
   TestValueType& operator=(TestValueType&& other) noexcept
   {
      val = std::move(other.val);
      return *this;
   }
};

struct HeavyTestValueType
{
   char val;
   std::vector<int> dummy;
   HeavyTestValueType(char c) : val(c), dummy(std::vector<int>(0, 0)) {}
   HeavyTestValueType(const HeavyTestValueType& other) : val(other.val), dummy(other.dummy) {}
   HeavyTestValueType(HeavyTestValueType&& other) noexcept : val(std::move(other.val)), dummy(std::move(other.dummy)) {}
   bool operator==(const HeavyTestValueType& other) const { return val == other.val; }
   HeavyTestValueType& operator=(const HeavyTestValueType& other)
   {
      val = other.val;
      dummy = other.dummy;
      return *this;
   }
   HeavyTestValueType& operator=(HeavyTestValueType&& other) noexcept
   {
      val = std::move(other.val);
      dummy = std::move(other.dummy);
      return *this;
   }
};

namespace std
{
template <>
class numeric_limits<TestKeyType>
{
public:
   static constexpr TestKeyType lowest() { return TestKeyType(numeric_limits<unsigned int>::lowest()); }
   // static constexpr TestKeyType lowest() { return TestKeyType(-250); }
};
}  // namespace std

namespace std
{
template <>
class numeric_limits<TestFloatKeyType>
{
public:
   static TestFloatKeyType lowest() { return TestFloatKeyType(numeric_limits<float>::lowest()); }
};
}  // namespace std

using Imap = step_function<TestKeyType, TestValueType>;

TEST_CASE("Randomized")
{
   std::random_device dev;
   std::mt19937 rng(dev());
   std::uniform_int_distribution<std::mt19937::result_type> dist6(1, 30);
   auto getRN = [&]() { return dist6(rng); };

   Imap map('A');
   for (int i = 1; i < 100000; ++i)
   {
      auto s = getRN();
      auto e = getRN();
      auto offset = getRN() % 4;
      auto val = 'A' + offset;

      auto pre_s = map[s - 1];
      auto e_unchanged = map[e];

      map.assign(s, e, val);
      if (s > e)
         continue;

      for (int k = s; k < e; ++k)
      {
         REQUIRE(map[k].val == val);
      }

      REQUIRE(map[s - 1] == pre_s);
      REQUIRE(map[e] == e_unchanged);

      REQUIRE(map.sanityCheck());
   }
}

TEST_CASE("EmptyRange")
{
   Imap m('A');
   m.assign(3, 3, 'B');
   REQUIRE(m.steps.count(3) == 0);

   m.assign(3, 2, 'B');
   REQUIRE(m.steps.count(2) == 0);
   REQUIRE(m.steps.count(3) == 0);

   REQUIRE(m.steps.size() == 0);

   REQUIRE(m.sanityCheck());
}

TEST_CASE("TrivialRange")
{
   Imap m('A');
   m.assign(1, 10, 'B');
   REQUIRE(m[0].val == 'A');
   for (int i = 1; i < 10; i++)
   {
      REQUIRE(m[i].val == 'B');
   }
   REQUIRE(m[10].val == 'A');
   REQUIRE(m[11].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("TrivialTwoRange")
{
   Imap m('A');
   m.assign(1, 3, 'B');
   m.assign(6, 8, 'C');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'A');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'A');
   REQUIRE(m[6].val == 'C');
   REQUIRE(m[7].val == 'C');
   REQUIRE(m[8].val == 'A');

   REQUIRE(m.steps.size() == 4);

   REQUIRE(m.sanityCheck());
}

TEST_CASE("OverwriteLowest")
{
   Imap m('A');
   m.assign(0, 10000, 'B');
   REQUIRE(m[0].val == 'B');
   REQUIRE(m[9999].val == 'B');
   REQUIRE(m[10000].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("Merge")
{
   Imap m('A');

   m.assign(0, 10, 'B');
   auto size_before = m.steps.size();
   m.assign(10, 20, 'B');
   REQUIRE(size_before == m.steps.size());

   REQUIRE(m[0].val == 'B');
   REQUIRE(m[10].val == 'B');
   REQUIRE(m[19].val == 'B');
   REQUIRE(m[20].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("FloatKey")
{
   step_function<TestFloatKeyType, TestValueType> m('A');
   m.assign(1.f, 5.f, 'B');
   REQUIRE(m[0.f].val == 'A');
   REQUIRE(m[.999999999f].val == 'B');
   REQUIRE(m[1.f].val == 'B');
   REQUIRE(m[4.999f].val == 'B');
   REQUIRE(m[5.f].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("OverlappingRangeComplete")
{
   Imap m('A');
   m.assign(3, 5, 'B');
   m.assign(1, 6, 'C');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'C');
   REQUIRE(m[2].val == 'C');
   REQUIRE(m[3].val == 'C');
   REQUIRE(m[4].val == 'C');
   REQUIRE(m[5].val == 'C');
   REQUIRE(m[6].val == 'A');

   REQUIRE(m.steps.size() == 2);

   REQUIRE(m.sanityCheck());
}

TEST_CASE("OverlappingRangeInner")
{
   Imap m('A');
   m.assign(1, 6, 'C');
   m.assign(3, 5, 'B');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'C');
   REQUIRE(m[2].val == 'C');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'B');
   REQUIRE(m[5].val == 'C');
   REQUIRE(m[6].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("OverlappingRangeSmallToLarge")
{
   Imap m('A');
   m.assign(1, 5, 'B');
   m.assign(3, 6, 'C');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'C');
   REQUIRE(m[4].val == 'C');
   REQUIRE(m[5].val == 'C');
   REQUIRE(m[6].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("OverlappingRangeLargeToSmall")
{
   Imap m('A');
   m.assign(3, 6, 'C');
   m.assign(1, 5, 'B');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'B');
   REQUIRE(m[5].val == 'C');
   REQUIRE(m[6].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("ExtendingRangeBegin")
{
   Imap m('A');
   m.assign(3, 5, 'B');
   m.assign(1, 4, 'B');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'B');
   REQUIRE(m[5].val == 'A');

   REQUIRE(m.steps.size() == 2);

   REQUIRE(m.sanityCheck());
}

TEST_CASE("ExtendingRangeBeginNoOverlap")
{
   Imap m('A');
   m.assign(3, 5, 'B');
   m.assign(1, 3, 'B');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'B');
   REQUIRE(m[5].val == 'A');

   REQUIRE(m.steps.size() == 2);

   REQUIRE(m.sanityCheck());
}

TEST_CASE("ExtendingRangeEnd")
{
   Imap m('A');
   m.assign(1, 5, 'B');
   m.assign(3, 6, 'B');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'B');
   REQUIRE(m[5].val == 'B');
   REQUIRE(m[6].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("ExtendingRangeEndNoOverlap")
{
   Imap m('A');
   m.assign(1, 5, 'B');
   m.assign(5, 6, 'B');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'B');
   REQUIRE(m[5].val == 'B');
   REQUIRE(m[6].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("ExtendingRangeBothBeginEnd")
{
   Imap m('A');
   m.assign(2, 3, 'B');
   m.assign(1, 5, 'B');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'B');
   REQUIRE(m[5].val == 'A');

   REQUIRE(m.steps.size() == 2);

   REQUIRE(m.sanityCheck());
}

TEST_CASE("OverwriteEndValueSafety")
{
   Imap m('A');
   m.assign(2, 5, 'B');
   m.assign(5, 8, 'C');
   m.assign(4, 5, 'A');

   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'A');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'C');
   REQUIRE(m[6].val == 'C');
   REQUIRE(m[7].val == 'C');
   REQUIRE(m[8].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("ReusingExistingRangeBothBeginEnd")
{
   Imap m('A');
   m.assign(1, 5, 'B');
   m.assign(2, 3, 'B');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'B');
   REQUIRE(m[5].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("ReusingEnd")
{
   Imap m('A');
   m.assign(1, 5, 'B');
   m.assign(4, 6, 'A');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'B');
   REQUIRE(m[2].val == 'B');
   REQUIRE(m[3].val == 'B');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("RestoringInitial")
{
   Imap m('A');
   m.assign(1, 5, 'B');
   m.assign(1, 5, 'A');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'A');
   REQUIRE(m[2].val == 'A');
   REQUIRE(m[3].val == 'A');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'A');

   REQUIRE(m.sanityCheck());

   REQUIRE(m.steps.empty());

   m.assign(1, 5, 'B');
   m.assign(0, 5, 'A');

   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'A');
   REQUIRE(m[2].val == 'A');
   REQUIRE(m[3].val == 'A');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'A');
   REQUIRE(m.sanityCheck());

   REQUIRE(m.steps.empty());

   m.assign(1, 5, 'B');
   m.assign(1, 6, 'A');

   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'A');
   REQUIRE(m[2].val == 'A');
   REQUIRE(m[3].val == 'A');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'A');
   REQUIRE(m[6].val == 'A');
   REQUIRE(m.sanityCheck());

   REQUIRE(m.steps.empty());

   m.assign(1, 5, 'B');
   m.assign(0, 6, 'A');

   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'A');
   REQUIRE(m[2].val == 'A');
   REQUIRE(m[3].val == 'A');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'A');
   REQUIRE(m[6].val == 'A');
   REQUIRE(m.sanityCheck());

   REQUIRE(m.steps.empty());
}

TEST_CASE("RestoringInitial2")
{
   Imap m('A');
   m.assign(1, 5, 'B');
   m.assign(0, 7, 'A');
   REQUIRE(m[0].val == 'A');
   REQUIRE(m[1].val == 'A');
   REQUIRE(m[2].val == 'A');
   REQUIRE(m[3].val == 'A');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'A');

   REQUIRE(m.sanityCheck());

   REQUIRE(m.steps.size() == 0);
}

TEST_CASE("VeryComplex")
{
   Imap m('A');
   m.assign(3, 6, 'B');
   m.assign(2, 5, 'C');
   m.assign(4, 7, 'A');

   REQUIRE(m[1].val == 'A');
   REQUIRE(m[2].val == 'C');
   REQUIRE(m[3].val == 'C');
   REQUIRE(m[4].val == 'A');
   REQUIRE(m[5].val == 'A');
   REQUIRE(m[6].val == 'A');
   REQUIRE(m[7].val == 'A');

   REQUIRE(m.sanityCheck());
}

TEST_CASE("Overflow")
{
   step_function<uint8_t, std::string> m("A");
   m.assign(1, 15, "B");
   REQUIRE(m[0] == "A");
   REQUIRE(m[1] == "B");
   REQUIRE(m[14] == "B");
   REQUIRE(m[15] == "A");
   REQUIRE(m[255] == "A");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
   REQUIRE(m[256] == "A");
#pragma GCC diagnostic pop

   m.assign(0, 10, "B");
   REQUIRE(m[0] == "B");
   REQUIRE(m[255] == "A");

   REQUIRE(m.sanityCheck());
}

// add randomized test
//
TEST_CASE("benchmarks")  // "[!benchmark]")
{
   BENCHMARK("10 insertions my map")
   {
      Imap map('A');

      map.assign(0, 10, 'B');
      map.assign(0, 15, 'C');
      map.assign(5, 20, 'B');
      map.assign(20, 30, 'D');
      map.assign(5, 25, 'F');

      map.assign(10, 11, 'B');
      map.assign(0, 15, 'C');
      map.assign(5, 20, 'B');
      map.assign(20, 30, 'D');
      map.assign(5, 25, 'F');

      return map;
   };

   constexpr int width = 1000;
   constexpr int valuevariance = 2;
   constexpr int seed = 1;
   constexpr int max_interval_width = 2;
   constexpr int runs = 100000;
   {
      // std::random_device dev;
      std::mt19937 rng(seed);
      std::uniform_int_distribution<std::mt19937::result_type> dist6(1, width);
      auto getRN = [&]() { return dist6(rng); };

      BENCHMARK("random insertions my map")
      {
         Imap map('A');
         for (int i = 1; i < runs; ++i)
         {
            auto s = getRN();
            auto e = s + ((s % max_interval_width) + 1);
            auto offset = s % valuevariance;
            auto val = 'A' + offset;

            map.assign(s, e, val);
         }

         return map;
      };
   }
};

void randomTest()
{
   std::random_device dev;
   std::mt19937 rng(dev());
   std::uniform_int_distribution<std::mt19937::result_type> dist6(1, 1000);
   auto getRN = [&]() { return dist6(rng); };

   Imap map('A');
   for (int i = 1; i < 100000000; ++i)
   {
      auto s = getRN();
      auto e = s + 1;
      auto offset = s % 2;
      auto val = 'A' + offset;

      auto s_ = map[s - 1];
      auto e_ = map[e];

      map.assign(s, e, val);

      if (map[s].val != val)
         throw std::runtime_error("");

      if (map[e - 1].val != val)
         throw std::runtime_error("");

      if (s_.val != map[s - 1].val || e_.val != map[e].val)
         throw std::runtime_error("");

      if (!map.sanityCheck())
         throw std::runtime_error("");
   }
}

void fixedTest()
{
   for (int i = 0; i < 10000000; ++i)
   {
      Imap map('A');

      map.assign(0, 10, 'B');
      map.assign(0, 15, 'C');
      map.assign(5, 20, 'B');
      map.assign(20, 30, 'D');
      map.assign(5, 25, 'F');

      map.assign(10, 11, 'B');
      map.assign(0, 15, 'C');
      map.assign(5, 20, 'B');
      map.assign(20, 30, 'D');
      map.assign(5, 25, 'F');
   }
}

int main()
{
   int result = Catch::Session().run();
   // randomTest();

   return 0;
}
