#include <type_traits>

template <typename... Args>
struct find_biggest;

//the biggest of one thing is that one thing
template <typename First>
struct find_biggest<First>
{
  typedef First type;
};

//the biggest of everything in Args and First
template <typename First, typename... Args>
struct find_biggest<First, Args...>
{
  typedef typename find_biggest<Args...>::type next;
  typedef typename std::conditional
  <
    sizeof(First) >= sizeof(next),
    First,
    next
  >::type type;
};
