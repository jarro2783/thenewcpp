#include <vector>
#include <cstdlib>

template <size_t... Args>
struct call_n_args
{
  template <typename F, typename Container>
  auto
  operator()(F f, const Container& c)
    -> decltype(f(c.at(Args)...))
  {
    return f(c.at(Args)...);
  }
};

template <typename F>
auto
foo(F f, const std::vector<int>& v)
  -> decltype(call_n_args<2,1,5,4>()(f, v))
{
  return call_n_args<2,1,5,4>()(f, v);
}

int
sum4(int a, int b, int c, int d)
{
  return a + b + c + d;
}

int main()
{
  std::vector<int> v{3,4,5,6,7,8,9};

  return foo(&sum4, v);
}
