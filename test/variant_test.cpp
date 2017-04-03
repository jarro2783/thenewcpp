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

  class NotMoveConstructible {
    public:
    
    NotMoveConstructible(NotMoveConstructible&&) = delete;
  };

  class CopyAssign {
    public:

    CopyAssign() = delete;

    CopyAssign(const CopyAssign&) = default;
    CopyAssign(CopyAssign&&) = default;

    CopyAssign&
    operator=(const CopyAssign&) = default;

    CopyAssign&
    operator=(CopyAssign&&) = delete;
  };

  class NotCopyAssign {
    public:

    NotCopyAssign() = default;
    NotCopyAssign(const NotCopyAssign&) = default;
    NotCopyAssign(NotCopyAssign&&) = default;

    NotCopyAssign&
    operator=(const NotCopyAssign&) = delete;

    NotCopyAssign&
    operator=(NotCopyAssign&&) = default;
  };

  class MoveConstructible {
    public:

    MoveConstructible(const MoveConstructible&) = delete;
    MoveConstructible(MoveConstructible&&) = default;
  };
}

typedef
  juice::variant<NotDefaultConstructible, int>
  VarNotDefaultConstructible;

typedef
  juice::variant<NotMoveConstructible, int>
  VarNotMoveConstructible;

typedef
  juice::variant<CopyAssign, int>
  VarCopyAssign;

typedef
  juice::variant<NotCopyAssign, int>
  VarNotCopyAssign;

typedef
  juice::variant<MoveConstructible, int>
  VarMoveConstructible;

TEST_CASE("Constructor availability", "[constructor]")
{
  REQUIRE((!std::is_default_constructible<NotDefaultConstructible>::value));
  REQUIRE((!std::is_default_constructible<VarNotDefaultConstructible>::value));
}

TEST_CASE("Copy/move availability", "[copy]")
{
  //move constructible
  REQUIRE((!std::is_move_constructible<NotMoveConstructible>::value));
  REQUIRE((!std::is_move_constructible<VarNotMoveConstructible>::value));
  REQUIRE((!std::is_move_assignable<VarNotMoveConstructible>::value));

  REQUIRE((!std::is_copy_assignable<VarNotMoveConstructible>::value));

  //only copy assignable
  REQUIRE((std::is_copy_assignable<CopyAssign>::value));
  REQUIRE((!std::is_move_assignable<CopyAssign>::value));

  REQUIRE((!std::is_default_constructible<VarCopyAssign>::value));
  REQUIRE((std::is_copy_assignable<VarCopyAssign>::value));
  REQUIRE((std::is_copy_constructible<VarCopyAssign>::value));
  REQUIRE((std::is_move_constructible<VarCopyAssign>::value));

  //not copy assignable
  REQUIRE((!std::is_copy_assignable<VarNotCopyAssign>::value));

  //only move constructible
  REQUIRE((std::is_move_constructible<MoveConstructible>::value));
  REQUIRE((std::is_move_constructible<VarMoveConstructible>::value));
  REQUIRE((!std::is_copy_constructible<VarMoveConstructible>::value));
  REQUIRE((!std::is_copy_assignable<VarMoveConstructible>::value));
  REQUIRE((!std::is_move_assignable<VarMoveConstructible>::value));
}

// There is nothing for Catch to test here because these are all
// compile time assertions.
TEST_CASE("Correct types", "[types]")
{
  int some_int = 42;

  juice::variant<int, std::string> x;

  REQUIRE(x.index() == 0);

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

struct TwoVisitor
{
  void
  operator()(int, float)
  {
  }

  void
  operator()(int, const std::string&)
  {
  }

  void
  operator()(char, float)
  {
  }

  void
  operator()(char, const std::string&)
  {
  }
};

TEST_CASE("Multi visitor", "[visitation]") {
  typedef juice::variant<int, char> V1;
  typedef juice::variant<std::string, float> V2;

  V1 x;
  V2 y;

  TwoVisitor visitor;

  juice::visit(visitor, x, y);
}

TEST_CASE("get_if values", "[get_if]")
{
  juice::variant<int, char> foo('a');
  auto value = juice::get_if<char>(&foo);

  REQUIRE((juice::get_if<int>(&foo) == nullptr));
  REQUIRE(value != nullptr);
  REQUIRE(*value == 'a');
}
