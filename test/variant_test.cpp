#include "catch.hpp"

#include <juice/variant.hpp>

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
    REQUIRE(v == "Hello world");
  }

  SECTION("convert to float") {
    v = 42.5f;
    REQUIRE(v.index() == 2);
    REQUIRE(juice::get<float>(v) == 42.5);
    REQUIRE(v == 42.5f);
  }
}
