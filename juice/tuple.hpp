#include <tuple>

namespace juice
{
  template <typename First, typename... Types>
  class Variant;

  static constexpr const size_t tuple_not_found = (size_t) -1;
  template <typename T, typename U> struct tuple_find;

  template <size_t N, typename T, typename First, typename... Types>
  struct tuple_find_helper :
  public tuple_find_helper<N+1, T, Types...>
  {
  };

  template <size_t N, typename T, typename... Types>
  struct tuple_find_helper<N, T, T, Types...>
  : public std::integral_constant<std::size_t, N>
  {
  };

  template <typename T, typename... Types>
  struct tuple_find<T, std::tuple<Types...>> 
  : public tuple_find_helper<0, T, Types...>
  {
  };

  template <typename T, typename... Types>
  struct tuple_find<T, Variant<Types...>> :
    public tuple_find<T, std::tuple<Types...>>
  {
  };
}
