/* A tagged union variant class.
   Copyright (C) 2013 Jarryd Beck

This file is part of Juice.

Distributed under the Boost Software License, Version 1.0

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

  The copyright notices in the Software and this entire statement, including
  the above license grant, this restriction and the following disclaimer,
  must be included in all copies of the Software, in whole or in part, and
  all derivative works of the Software, unless such copies or derivative
  works are solely in the form of machine-executable object code generated by
  a source language processor.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
  FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.

*/

/**
 * @file variant.hpp
 * A tagged union. This effectively has the same functionality as
 * boost::variant, but replaces it with C++11 features.
 */

#ifndef JUICE_VARIANT_HPP_INCLUDED
#define JUICE_VARIANT_HPP_INCLUDED

#include <cassert>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

#include <juice/mpl.hpp>

namespace Juice
{
  namespace MPL
  {
    struct true_ {};
    struct false_ {};
  }

  template <typename R = void>
  class static_visitor
  {
    public:
    typedef R result_type;

    //so that it can't be instantiated
    protected:
    ~static_visitor() = default;
  };

  template <typename T>
  class recursive_wrapper
  {
    public:
    ~recursive_wrapper()
    {
      delete m_t;
    }

    template 
    <
      typename U,
      typename Dummy = 
        typename std::enable_if<std::is_convertible<U, T>::value, U>::type
    >
    recursive_wrapper(
      const U& u)
    : m_t(new T(u))
    {
    }

    template 
    <
      typename U,
      typename Dummy =
        typename std::enable_if<std::is_convertible<U, T>::value, U>::type
    >
    recursive_wrapper(U&& u)
    : m_t(new T(std::forward<U>(u))) { }

    recursive_wrapper(const recursive_wrapper& rhs)
    : m_t(new T(rhs.get())) { }

    recursive_wrapper(recursive_wrapper&& rhs)
    : m_t(rhs.m_t)
    {
      rhs.m_t = nullptr;
    }

    recursive_wrapper&
    operator=(const recursive_wrapper& rhs)
    {
      assign(rhs.get());
      return *this;
    }

    recursive_wrapper&
    operator=(recursive_wrapper&& rhs)
    {
      delete m_t;
      m_t = rhs.m_t;
      rhs.m_t = nullptr;
      return *this;
    }

    recursive_wrapper&
    operator=(const T& t)
    {
      assign(t);
      return *this;
    }

    recursive_wrapper&
    operator=(T&& t)
    {
      assign(std::move(t));
      return *this;
    }

    bool
    operator==(const recursive_wrapper& rhs) const
    {
      return *m_t == *rhs.m_t;
    }

    T& get() { return *m_t; }
    const T& get() const { return *m_t; }

    private:
    T* m_t;

    template <typename U>
    void
    assign(U&& u)
    {
      *m_t = std::forward<U>(u);
    }
  };

  namespace detail
  {
    template <typename T, typename Internal>
    T&
    get_value(T& t, const Internal&)
    {
      return t;
    }

    template <typename T>
    T&
    get_value(recursive_wrapper<T>& t, const MPL::false_&)
    {
      return t.get();
    }

    template <typename T>
    const T&
    get_value(const recursive_wrapper<T>& t, const MPL::false_&)
    {
      return t.get();
    }

    template <typename Visitor, typename Visitable>
    struct BinaryVisitor
    {
      typedef typename std::remove_reference<Visitor>::type::result_type
        result_type;

      BinaryVisitor(Visitor&& visitor, Visitable&& visitable)
      : v(visitor)
      , visitable(visitable)
      {
      }

      template <typename T>
      result_type
      operator()(T&& t)
      {
        return apply_visitor(v, visitable, std::forward<T>(t));
      }

      private:

      Visitor& v;
      Visitable& visitable;
    };

  }    

  template 
  <
    typename Internal,
    typename T, 
    typename Storage, 
    typename Visitor, 
    typename... Args
  >
  typename std::remove_reference<Visitor>::type::result_type
  visitor_caller(Internal&& internal, 
    Storage&& storage, Visitor&& visitor, Args&&... args)
  {
    typedef typename std::conditional
    <
      std::is_const<
        typename std::remove_pointer<
          typename std::remove_reference<Storage>::type
        >::type
      >::value,
      const T,
      T
    >::type ConstType;

    return visitor(detail::get_value(*reinterpret_cast<ConstType*>(storage), 
      internal), std::forward<Args>(args)...);
  }

  template <typename First, typename... Types>
  class Variant
  {
    private:

    template <typename... AllTypes>
    struct do_visit
    {
      template 
      <
        typename Internal, 
        typename VoidPtrCV, 
        typename Visitor, 
        typename... Args
      >
      typename std::remove_reference<Visitor>::type::result_type
      operator()
      (
        Internal&& internal,
        size_t which, 
        VoidPtrCV&& storage, 
        Visitor&& visitor,
        Args&&... args
      )
      {
        typedef typename std::remove_reference<Visitor>::type::result_type
          (*whichCaller)(Internal&&, VoidPtrCV&&, Visitor&&, Args&&...);

        static whichCaller callers[sizeof...(AllTypes)] =
          {
            &visitor_caller<Internal&&, AllTypes,
              VoidPtrCV&&, Visitor, Args&&...>...
          }
        ;

        assert(which >= 0 && which < sizeof...(AllTypes));

        return (*callers[which])
          (
            std::forward<Internal>(internal),
            std::forward<VoidPtrCV>(storage), 
            std::forward<Visitor>(visitor), 
            std::forward<Args>(args)...
          );
      }
    };

    template <typename T>
    struct Sizeof
    {
      static constexpr size_t value = sizeof(T);
    };

    template <typename T>
    struct Alignof
    {
      static constexpr size_t value = alignof(T);
    };

    //size = max of size of each thing
    static constexpr size_t m_size = 
      max
      <
        Sizeof,
        First,
        Types...
      >::value;

    struct constructor
    {
      typedef void result_type;

      constructor(Variant& self)
      : m_self(self)
      {
      }

      template <typename T>
      void
      operator()(const T& rhs) const
      {
        m_self.construct(rhs);
      }

      private:
      Variant& m_self;
    };

    struct move_constructor
    {
      typedef void result_type;
      
      move_constructor(Variant& self)
      : m_self(self)
      {
      }

      template <typename T>
      void
      operator()(T& rhs) const
      {
        m_self.construct(std::move(rhs));
      }

      private:
      Variant& m_self;
    };

    struct assigner
    {
      typedef void result_type;

      assigner(Variant& self, int rhs_which)
      : m_self(self), m_rhs_which(rhs_which)
      {
      }

      template <typename Rhs>
      void
      operator()(const Rhs& rhs) const
      {
        if (m_self.which() == m_rhs_which)
        {
          //the types are the same, so just assign into the lhs
          *reinterpret_cast<Rhs*>(m_self.address()) = rhs;
        }
        else
        {
          Rhs tmp(rhs);
          m_self.destroy(); //nothrow
          m_self.construct(std::move(tmp)); //nothrow (please)
        }
      }

      private:
      Variant& m_self;
      int m_rhs_which;
    };
    
    struct move_assigner
    {
      typedef void result_type;

      move_assigner(Variant& self, int rhs_which)
      : m_self(self), m_rhs_which(rhs_which)
      {
      }

      template <typename Rhs>
      void
      operator()(Rhs& rhs) const
      {
        typedef typename std::remove_const<Rhs>::type RhsNoConst;
        if (m_self.which() == m_rhs_which)
        {
          //the types are the same, so just assign into the lhs
          *reinterpret_cast<RhsNoConst*>(m_self.address()) = std::move(rhs);
        }
        else
        {
          m_self.destroy(); //nothrow
          m_self.construct(std::move(rhs)); //nothrow (please)
        }
      }

      private:
      Variant& m_self;
      int m_rhs_which;
    };

    struct equality
    {
      typedef bool result_type;

      equality(const Variant& self)
      : m_self(self)
      {
      }

      template <typename Rhs>
      bool
      operator()(Rhs& rhs) const
      {
        return *reinterpret_cast<Rhs*>(m_self.address()) == rhs;
      }

      private:
      const Variant& m_self;
    };

    struct destroyer
    {
      typedef void result_type;

      template <typename T>
      void
      operator()(T& t) const
      {
        t.~T();
      }
    };

    template <size_t Which, typename... MyTypes>
    struct initialiser;

    template <size_t Which, typename Current, typename... MyTypes>
    struct initialiser<Which, Current, MyTypes...> 
      : public initialiser<Which+1, MyTypes...>
    {
      typedef initialiser<Which+1, MyTypes...> base;
      using base::initialise;

      static void 
      initialise(Variant& v, Current&& current)
      {
        v.construct(std::move(current));
        v.indicate_which(Which);
      }

      static void
      initialise(Variant& v, const Current& current)
      {
        v.construct(current);
        v.indicate_which(Which);
      }
    };

    template <size_t Which>
    struct initialiser<Which>
    {
      //this should never match
      void initialise();
    };

    public:

    Variant()
    {
      //try to construct First
      //if this fails then First is not default constructible
      construct(First());
      indicate_which(0);
    }

    ~Variant()
    {
      destroy();
    }

    //enable_if disables this function if we are constructing with a Variant.
    //Unfortunately, this becomes Variant(Variant&) which is a better match
    //than Variant(const Variant& rhs), so it is chosen. Therefore, we disable
    //it.
    template 
    <
      typename T, 
      typename Dummy = 
        typename std::enable_if
        <
          !std::is_same
          <
            typename std::remove_reference<Variant<First, Types...>>::type,
            typename std::remove_reference<T>::type
          >::value,
          T
        >::type
    >
    Variant(T&& t)
    {
       static_assert(
          !std::is_same<Variant<First, Types...>&, T>::value, 
          "why is Variant(T&&) instantiated with a Variant?");

      //compile error here means that T is not unambiguously convertible to
      //any of the types in (First, Types...)
      initialiser<0, First, Types...>::initialise(*this, std::forward<T>(t));
    }

    Variant(const Variant& rhs)
    {
      rhs.apply_visitor_internal(constructor(*this));
      indicate_which(rhs.which());
    }

    Variant(Variant&& rhs)
    {
      rhs.apply_visitor_internal(move_constructor(*this));
      indicate_which(rhs.which());
    }

    Variant& operator=(const Variant& rhs)
    {
      if (this != &rhs)
      {
        rhs.apply_visitor_internal(assigner(*this, rhs.which()));
        indicate_which(rhs.which());
      }
      return *this;
    }

    Variant& operator=(Variant&& rhs)
    {
      if (this != &rhs)
      {
        rhs.apply_visitor_internal(move_assigner(*this, rhs.which()));
        indicate_which(rhs.which());
      }
      return *this;
    }

    bool
    operator==(const Variant& rhs) const
    {
      if (which() != rhs.which())
      {
        return false;
      }

      return rhs.apply_visitor_internal(equality(*this));
    }

    int which() const {return m_which;}

    template <typename Internal, typename Visitor, typename... Args>
    typename std::remove_reference<Visitor>::type::result_type
    apply_visitor(Visitor&& visitor, Args&&... args)
    {
      return do_visit<First, Types...>()(Internal(), m_which, &m_storage,
        std::forward<Visitor>(visitor), std::forward<Args>(args)...);
    }

    template <typename Internal, typename Visitor, typename... Args>
    typename std::remove_reference<Visitor>::type::result_type
    apply_visitor(Visitor&& visitor, Args&&... args) const
    {
      return do_visit<First, Types...>()(Internal(), m_which, &m_storage,
        std::forward<Visitor>(visitor), std::forward<Args>(args)...);
    }

    private:

    typename 
      std::aligned_storage<m_size, max<Alignof, First, Types...>::value>::type
      m_storage;

    int m_which;

    static std::function<void(void*)> m_handlers[1 + sizeof...(Types)];

    void indicate_which(int which) {m_which = which;}

    void* address() {return &m_storage;}
    const void* address() const {return &m_storage;}

    template <typename Visitor>
    typename Visitor::result_type
    apply_visitor_internal(Visitor&& visitor)
    {
      return apply_visitor<MPL::true_, Visitor>(std::forward<Visitor>(visitor));
    }

    template <typename Visitor>
    typename Visitor::result_type
    apply_visitor_internal(Visitor&& visitor) const
    {
      return apply_visitor<MPL::true_, Visitor>(std::forward<Visitor>(visitor));
    }

    void
    destroy()
    {
      apply_visitor_internal(destroyer());
    }

    template <typename T>
    void
    construct(T&& t)
    {
      typedef typename std::remove_reference<T>::type type;
      new(&m_storage) type(std::forward<T>(t));
    }
  };

  struct bad_get : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "bad_get";
    }
  };

  template <typename T>
  struct get_visitor
  {
    typedef T* result_type;

    result_type
    operator()(T& val) const
    {
      return &val;
    }

    template <typename U>
    result_type
    operator()(const U&) const
    {
      return nullptr;
    }
  };

  template <typename Visitor, typename Visitable, typename... Args>
  typename std::remove_reference<Visitor>::type::result_type
  apply_visitor(Visitor&& visitor, Visitable& visitable, Args&&... args)
  {
    return visitable.template apply_visitor<MPL::false_>
      (std::forward<Visitor>(visitor), std::forward<Args>(args)...);
  }

  template <typename T, typename First, typename... Types>
  T*
  get(Variant<First, Types...>* var)
  {
    return apply_visitor(get_visitor<T>(), *var);
  }

  template <typename T, typename First, typename... Types>
  const T*
  get(const Variant<First, Types...>* var)
  {
    return apply_visitor(get_visitor<const T>(), *var);
  }

  template <typename T, typename First, typename... Types>
  T&
  get (Variant<First, Types...>& var)
  {
    T* t = apply_visitor(get_visitor<T>(), var);
    if (t == nullptr){throw bad_get();}

    return *t;
  }

  template <typename T, typename First, typename... Types>
  const T&
  get (const Variant<First, Types...>& var)
  {
    const T* t = apply_visitor(get_visitor<const T>(), var);
    if (t == nullptr) {throw bad_get();}

    return *t;
  }

  struct visitor_applier
  {
    template <typename Visitor, typename Visitable, typename... Args>
    auto
    operator()(Visitor&& visitor, Visitable&& visitable, Args&&... args)
    -> decltype
      (
        apply_visitor
        (
          std::forward<Visitor>(visitor),
          std::forward<Visitable>(visitable),
          std::forward<Args>(args)...
        )
      )
    {
      return apply_visitor
      (
        std::forward<Visitor>(visitor),
        std::forward<Visitable>(visitable),
        std::forward<Args>(args)...
      );
    }
  };

  template <typename T, typename V>
  bool
  variant_is_type(const V& v)
  {
    return get<T>(&v) != nullptr;
  }

  template 
  <
    typename Visitor,
    typename Visitable1,
    typename Visitable2
  >
  typename std::remove_reference<Visitor>::type::result_type
  apply_visitor_binary(Visitor&& visitor, Visitable1&& v1, Visitable2&& v2)
  {
    detail::BinaryVisitor<Visitor, Visitable1> v{
      std::forward<Visitor>(visitor), 
      std::forward<Visitable1>(v1)
    };

    return apply_visitor(v, std::forward<Visitable2>(v2));
  }

}

#endif
