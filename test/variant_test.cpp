#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <juice/variant.hpp>

TEST_CASE("Emplace correct position", "[emplace]")
{
  juice::variant<int, int> v;
  v.emplace<1>(0);
  REQUIRE(v.index() == 1);
}
