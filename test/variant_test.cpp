#include <juice/variant.hpp>

#include "catch.hpp"
#include "test_facilities.hpp"

// Catch must override == and we can't seem to write v == expr. We need to
// use v.operator==.

TEST_CASE("Destruct objects", "[destructor]")
{
  bool destructed = false;
  {
    juice::variant<DestructTracker, int>(
      juice::emplaced_type<DestructTracker>, destructed);
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

  REQUIRE(a < b);
  REQUIRE(a < c);
  REQUIRE(c < MyVariant("Hi"));
  REQUIRE(b > a);
  REQUIRE(c > a);
}
