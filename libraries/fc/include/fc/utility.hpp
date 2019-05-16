#pragma once
#include <stdint.h>
#include <new>

#ifdef _MSC_VER
#pragma warning(disable: 4482) // nonstandard extension used enum Name::Val, standard in C++11
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

namespace fc {
  using std::size_t;
  typedef decltype(nullptr) nullptr_t;

  template<typename T> struct deduce           { typedef T type; };
  template<typename T> struct deduce<T&>       { typedef T type; };
  template<typename T> struct deduce<const T&> { typedef T type; };
  template<typename T> struct deduce<T&&>      { typedef T type; };
  template<typename T> struct deduce<const T&&>{ typedef T type; };

  struct true_type  { enum _value { value = 1 }; };
  struct false_type { enum _value { value = 0 }; };

  namespace detail {
    template<typename T> fc::true_type is_class_helper(void(T::*)());
    template<typename T> fc::false_type is_class_helper(...);
  }

  template<typename T>
  struct is_class { typedef decltype(detail::is_class_helper<T>(0)) type; enum value_enum { value = type::value }; };

}
