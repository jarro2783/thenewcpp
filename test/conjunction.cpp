#include <juice/conjunction.hpp>

#include "catch.hpp"

template <bool... B>
using cj = juice::conjunction<B...>;

template <bool... B>
auto cjv = juice::conjunction_v<B...>;

TEST_CASE("True", "[true]")
{
  CHECK(cjv<true>);
  CHECK((cjv<true, true>));
  CHECK((cjv<true, true, true>));
}

TEST_CASE("False", "[false]")
{
  CHECK(!cjv<false>);
  CHECK(!(cjv<false, false>));
  CHECK(!(cjv<false, false, false>));
}

TEST_CASE("Mixed", "[true, false]")
{
  CHECK(!(cjv<false, true>));
  CHECK(!(cjv<true, false>));
  CHECK(!(cjv<true, true, false>));
  CHECK(!(cjv<false, true, true>));
}
