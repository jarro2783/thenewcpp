#include <juice/variant.hpp>

#include "catch.hpp"
#include "test_facilities.hpp"

// Catch must override == and we can't seem to write v == expr. We need to
// use v.operator==.

namespace
{
  template <typename A, typename B>
  struct types_equal :
    public std::integral_constant<bool, false>
  {
  };

  template <typename A>
  struct types_equal<A, A> :
    public std::integral_constant<bool, true>
  {
  };

  template <typename A, typename B>
  constexpr bool types_equal_v = types_equal<A, B>::value;

  template <bool B>
  struct validate
    : public std::integral_constant<bool, true>
  {
    static_assert(B, "Validation Error");
  };

  template <bool B>
  constexpr bool validate_v = validate<B>::value;
}

// There is nothing for Catch to test here because these are all
// compile time assertions.
TEST_CASE("Correct types", "[types]")
{
  juice::variant<int, std::string> x;
  REQUIRE((
    validate_v<types_equal_v<decltype(juice::get<int>(x)), int&>> &&
    validate_v<types_equal_v<decltype(juice::get<int>(std::move(x))), int&&>>
  ));
}

TEST_CASE("Destruct objects", "[destructor]")
{
  bool destructed = false;
  {
    juice::variant<DestructTracker, int>(
      juice::in_place_type<DestructTracker>, destructed);
  }

  REQUIRE(destructed);
}

TEST_CASE("Emplace correct position", "[emplace]")
{
  juice::variant<int, int> v;

  v.emplace<1>(42);
  REQUIRE(v.index() == 1);
  REQUIRE(juice::get<1>(v) == 42);

  v.emplace<0>(100);
  REQUIRE(v.index() == 0);
  REQUIRE(juice::get<0>(v) == 100);
}

TEST_CASE("Assign string", "[assign]")
{
  juice::variant<int, std::string, float> v("Hello world");
  REQUIRE(v.index() == 1);
  REQUIRE(v.operator==("Hello world"));
}

TEST_CASE("Get correct type", "[get]")
{
  typedef juice::variant<int, std::string, float> MyVariant;

  MyVariant v(5);
  REQUIRE(v.index() == 0);
  REQUIRE(juice::get<int>(v) == 5);
  REQUIRE(juice::get<0>(v) == 5);

  SECTION("convert to string") {
    v = std::string("Hello world");
    REQUIRE(v.index() == 1);
    REQUIRE(juice::get<std::string>(v) == "Hello world");
    REQUIRE(v.operator==("Hello world"));
  }

  SECTION("convert to float") {
    v = 42.5f;
    REQUIRE(v.index() == 2);
    REQUIRE(juice::get<float>(v) == 42.5);
    REQUIRE(v == 42.5f);
  }
}

TEST_CASE("Comparison", "[compare]") {
  typedef juice::variant<int, std::string> MyVariant;

  MyVariant a(4);
  MyVariant b(5);
  MyVariant c("Hello world");
  MyVariant d(5);

  REQUIRE(a < b);
  REQUIRE(a < c);
  REQUIRE(c < MyVariant("Hi"));
  REQUIRE(b > a);
  REQUIRE(c > a);
  REQUIRE(b == d);
}
