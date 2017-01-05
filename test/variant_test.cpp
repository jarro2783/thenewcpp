#include <list>
#include <type_traits>
#include <vector>

#include <juice/variant.hpp>

#include "catch.hpp"
#include "test_facilities.hpp"

// Catch must override == and we can't seem to write v == expr. We need to
// use v.operator==.

namespace
{
  template <typename A, typename B>
  struct types_equal
  {
    static constexpr auto value =
      std::integral_constant<bool, std::is_same<A, B>::value>::value;
    static_assert(value, "Types not equal");
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


  template <size_t I, typename Variant, typename... Args,
    typename = decltype(std::declval<Variant>(). template
      emplace<I>(std::declval<Args>()...))
  >
  bool
  emplace_defined(Variant&&, Args&&...)
  {
    return true;
  }

  template <size_t I, typename Variant, typename... Args>
  bool
  emplace_defined(const Variant&, Args&&...)
  {
    return false;
  }

  class NotDefaultConstructible {
    public:

    NotDefaultConstructible() = delete;
  };
}

TEST_CASE("Constructor availability", "[constructor]")
{
  typedef juice::variant<NotDefaultConstructible, int> VarNotDefaultConstructible;
  REQUIRE((!std::is_default_constructible<NotDefaultConstructible>::value));
  REQUIRE((!std::is_default_constructible<VarNotDefaultConstructible>::value));
}

// There is nothing for Catch to test here because these are all
// compile time assertions.
TEST_CASE("Correct types", "[types]")
{
  int some_int = 42;

  juice::variant<int, std::string> x;
  const juice::variant<int, std::string> cx;
  juice::variant<int&&, char&&> move_refs(std::move(some_int));
  juice::variant<int&, char&> refs(some_int);

  REQUIRE((
    types_equal_v<decltype(juice::get<int>(x)), int&> &&
    types_equal_v<
      decltype(juice::get<int>(std::move(x))), int&&> &&
    types_equal_v<
      decltype(juice::get<int&&>(std::move(x))), int&&> &&
    types_equal_v<decltype(juice::get<int&>(refs)), int&> &&

    types_equal_v<decltype(juice::get_if<0>(&x)), int*> &&
    types_equal_v<decltype(juice::get_if<1>(&x)), std::string*> &&
    types_equal_v<decltype(juice::get_if<int>(&x)), int*> &&
    types_equal_v<decltype(juice::get_if<std::string>(&x)), std::string*> &&

    //const values
    types_equal_v<decltype(juice::get_if<0>(&cx)), const int*> &&
    types_equal_v<decltype(juice::get_if<1>(&cx)), const std::string*> &&
    types_equal_v<decltype(juice::get_if<int>(&cx)), const int*> &&
    types_equal_v<decltype(juice::get_if<std::string>(&cx)),
      const std::string*>
  ));

  //TODO: This should pass when ~variant is a trivial destructor
  constexpr juice::variant<int, char> y(5);
}

TEST_CASE("Destruct objects", "[destructor]")
{
  bool destructed = false;
  {
    juice::variant<DestructTracker, int> track_me(
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

TEST_CASE("Emplace correct type", "[emplace]")
{
  juice::variant<int, char, std::string> v;

  SECTION("emplace int") {
    v.emplace<int>(5);
    REQUIRE(v.index() == 0);
    REQUIRE(juice::get<int>(v) == 5);
    REQUIRE(juice::get<0>(v) == 5);
  }

  SECTION("emplace char") {
    v.emplace<char>('a');
    REQUIRE(v.index() == 1);
    REQUIRE(juice::get<char>(v) == 'a');
    REQUIRE(juice::get<1>(v) == 'a');
  }

  SECTION("emplace string") {
    v.emplace<std::string>("hello world");
    REQUIRE(v.index() == 2);
    REQUIRE(juice::get<std::string>(v) == "hello world");
    REQUIRE(juice::get<2>(v) == "hello world");
  }
}

TEST_CASE("Assign string", "[assign]")
{
  juice::variant<int, std::string, float> v("Hello world");
  REQUIRE(v.index() == 1);
  REQUIRE(v == decltype(v)("Hello world"));
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
    REQUIRE(v == decltype(v)("Hello world"));
  }

  SECTION("convert to float") {
    v = 42.5f;
    REQUIRE(v.index() == 2);
    REQUIRE(juice::get<float>(v) == 42.5);
    REQUIRE(v == MyVariant(42.5f));
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

TEST_CASE("Emplace with initializer_list", "[emplace]") {
  typedef juice::variant<std::vector<int>, std::list<int>> MyVariant;

  MyVariant v;

  v.emplace<0>({0, 1, 2, 3}, std::allocator<int>());
  REQUIRE(v.index() == 0);
  auto& vec = juice::get<0>(v);
  REQUIRE((vec == std::vector<int>{0, 1, 2, 3}));

  v.emplace<std::list<int>>({0, 1, 2});
  REQUIRE(v.index() == 1);
  REQUIRE((juice::get<std::list<int>>(v) == std::list<int>{0, 1, 2}));
}

TEST_CASE("Emplace disabled", "[emplace]") {
  typedef juice::variant<int, char, std::string> V;

  V v{};

  REQUIRE((!std::is_constructible<int, int&, int&>::value));

  REQUIRE((emplace_defined<0>(v, 1)));
  REQUIRE((!emplace_defined<0>(v, 1, 2)));
  REQUIRE((emplace_defined<1>(v, 'c')));
  REQUIRE((emplace_defined<2>(v, 3, '4')));
  REQUIRE((!emplace_defined<2>(v, v)));
}
