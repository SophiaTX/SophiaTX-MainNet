/** This source adapted from https://github.com/kmicklas/variadic-static_variant. Now available at https://github.com/kmicklas/variadic-variant.
 *
 * Copyright (C) 2013 Kenneth Micklas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **/
#pragma once
#include <stdexcept>
#include <typeinfo>
#include <fc/exception/exception.hpp>

namespace fc {

// Implementation details, the user should not import this:
namespace impl {

template<int64_t N, typename... Ts>
struct storage_ops;

template<typename X, typename... Ts>
struct position;

template<typename... Ts>
struct type_info;

template<typename StaticVariant>
struct copy_construct
{
   typedef void result_type;
   StaticVariant& sv;
   copy_construct( StaticVariant& s ):sv(s){}
   template<typename T>
   void operator()( const T& v )const
   {
      sv.init(v);
   }
};

template<typename StaticVariant>
struct move_construct
{
   typedef void result_type;
   StaticVariant& sv;
   move_construct( StaticVariant& s ):sv(s){}
   template<typename T>
   void operator()( T& v )const
   {
      sv.init( std::move(v) );
   }
};

template<int64_t N, typename T, typename... Ts>
struct storage_ops<N, T&, Ts...> {
    static void del(int64_t n, void *data) {}
    static void con(int64_t n, void *data) {}

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, void *data, visitor& v) {}

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, void *data, const visitor& v) {}

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, const void *data, visitor& v) {}

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, const void *data, const visitor& v) {}
};

template<int64_t N, typename T, typename... Ts>
struct storage_ops<N, T, Ts...> {
    static void del(int64_t n, void *data) {
        if(n == N) reinterpret_cast<T*>(data)->~T();
        else storage_ops<N + 1, Ts...>::del(n, data);
    }
    static void con(int64_t n, void *data) {
        if(n == N) new(reinterpret_cast<T*>(data)) T();
        else storage_ops<N + 1, Ts...>::con(n, data);
    }

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, void *data, visitor& v) {
        if(n == N) return v(*reinterpret_cast<T*>(data));
        else return storage_ops<N + 1, Ts...>::apply(n, data, v);
    }

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, void *data, const visitor& v) {
        if(n == N) return v(*reinterpret_cast<T*>(data));
        else return storage_ops<N + 1, Ts...>::apply(n, data, v);
    }

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, const void *data, visitor& v) {
        if(n == N) return v(*reinterpret_cast<const T*>(data));
        else return storage_ops<N + 1, Ts...>::apply(n, data, v);
    }

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, const void *data, const visitor& v) {
        if(n == N) return v(*reinterpret_cast<const T*>(data));
        else return storage_ops<N + 1, Ts...>::apply(n, data, v);
    }
};

template<int64_t N>
struct storage_ops<N> {
    static void del(int64_t n, void *data) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid.");
    }
    static void con(int64_t n, void *data) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }

    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, void *data, visitor& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, void *data, const visitor& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, const void *data, visitor& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
    template<typename visitor>
    static typename visitor::result_type apply(int64_t n, const void *data, const visitor& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
};

template<typename X>
struct position<X> {
    static const int64_t pos = -1;
};

template<typename X, typename... Ts>
struct position<X, X, Ts...> {
    static const int64_t pos = 0;
};

template<typename X, typename T, typename... Ts>
struct position<X, T, Ts...> {
    static const int64_t pos = position<X, Ts...>::pos != -1 ? position<X, Ts...>::pos + 1 : -1;
};

template<typename T, typename... Ts>
struct type_info<T&, Ts...> {
    static const bool no_reference_types = false;
    static const bool no_duplicates = position<T, Ts...>::pos == -1 && type_info<Ts...>::no_duplicates;
    static const size_t size = type_info<Ts...>::size > sizeof(T&) ? type_info<Ts...>::size : sizeof(T&);
    static const size_t count = 1 + type_info<Ts...>::count;
};

template<typename T, typename... Ts>
struct type_info<T, Ts...> {
    static const bool no_reference_types = type_info<Ts...>::no_reference_types;
    static const bool no_duplicates = position<T, Ts...>::pos == -1 && type_info<Ts...>::no_duplicates;
    static const size_t size = type_info<Ts...>::size > sizeof(T) ? type_info<Ts...>::size : sizeof(T&);
    static const size_t count = 1 + type_info<Ts...>::count;
};

template<>
struct type_info<> {
    static const bool no_reference_types = true;
    static const bool no_duplicates = true;
    static const size_t count = 0;
    static const size_t size = 0;
};

template<typename TTag>
size_t size( TTag )
{
   return 0;
}

template<typename TTag, typename A, typename...Ts>
size_t size( TTag tag )
{
   if (tag <= 0)
   {
      return sizeof(A);
   }
   return size<TTag, Ts...>( --tag );
}
class dynamic_storage
{
   char* storage;
public:
   dynamic_storage();
   ~dynamic_storage();
   void* data() const;
   void alloc( size_t size );
   void release();
};


} // namespace impl

template<typename... Types>
class static_variant {
public:
    using tag_type = int64_t;

   protected:
    static_assert(impl::type_info<Types...>::no_reference_types, "Reference types are not permitted in static_variant.");
    static_assert(impl::type_info<Types...>::no_duplicates, "static_variant type arguments contain duplicate types.");

    template<typename X>
    using type_in_typelist = typename std::enable_if<impl::position<X, Types...>::pos != -1, X>::type; // type is in typelist of static_variant.

    tag_type _tag;
    impl::dynamic_storage storage;

    template<typename X, typename = type_in_typelist<X>>
    void init(const X& x) {
        _tag = impl::position<X, Types...>::pos;
        storage.alloc( sizeof(X) );
        new(storage.data()) X(x);
    }

    template<typename X, typename = type_in_typelist<X>>
    void init(X&& x) {
        _tag = impl::position<X, Types...>::pos;
        storage.alloc( sizeof(X) );
        new(storage.data()) X( std::move(x) );
    }

    void init_from_tag(tag_type tag)
    {
       FC_ASSERT( tag >= 0 );
       FC_ASSERT( tag < count() );
       _tag = tag;
       storage.alloc( impl::size<tag_type, Types...>( tag ) );
       impl::storage_ops<0, Types...>::con(_tag, storage.data());
    }

    void clean()
    {
       impl::storage_ops<0, Types...>::del(_tag, storage.data() );
       storage.release();
    }

    template<typename StaticVariant>
    friend struct impl::copy_construct;
    template<typename StaticVariant>
    friend struct impl::move_construct;
public:
    template<typename X, typename = type_in_typelist<X>>
    struct tag
    {
        static const int64_t value = impl::position<X, Types...>::pos;
    };

    static_variant()
    {
       init_from_tag(0);
    }

    template<typename... Other>
    static_variant( const static_variant<Other...>& cpy )
    {
       cpy.visit( impl::copy_construct<static_variant>(*this) );
    }
    static_variant( const static_variant& cpy )
    {
       cpy.visit( impl::copy_construct<static_variant>(*this) );
    }

    static_variant( static_variant&& mv )
    {
       mv.visit( impl::move_construct<static_variant>(*this) );
    }

    template<typename X, typename = type_in_typelist<X>>
    static_variant(const X& v) {
        init(v);
    }

    ~static_variant() {
       clean();
    }


    template<typename X, typename = type_in_typelist<X>>
    static_variant& operator=(const X& v) {
        clean();
        init(v);
        return *this;
    }

    static_variant& operator=( const static_variant& v )
    {
       if( this == &v ) return *this;
       clean();
       v.visit( impl::copy_construct<static_variant>(*this) );
       return *this;
    }
    static_variant& operator=( static_variant&& v )
    {
       if( this == &v ) return *this;
       clean();
       v.visit( impl::move_construct<static_variant>(*this) );
       return *this;
    }
    friend bool operator == ( const static_variant& a, const static_variant& b )
    {
       return a.which() == b.which();
    }
    friend bool operator < ( const static_variant& a, const static_variant& b )
    {
       return a.which() < b.which();
    }

    template<typename X, typename = type_in_typelist<X>>
    X& get() {
        if(_tag == impl::position<X, Types...>::pos) {
            return *reinterpret_cast<X*>(storage.data());
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
        }
    }
    template<typename X, typename = type_in_typelist<X>>
    const X& get() const {
        if(_tag == impl::position<X, Types...>::pos) {
            return *reinterpret_cast<X*>(storage.data());
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
        }
    }
    template<typename visitor>
    typename visitor::result_type visit(visitor& v) {
        return impl::storage_ops<0, Types...>::apply(_tag, storage.data(), v);
    }

    template<typename visitor>
    typename visitor::result_type visit(const visitor& v) {
        return impl::storage_ops<0, Types...>::apply(_tag, storage.data(), v);
    }

    template<typename visitor>
    typename visitor::result_type visit(visitor& v)const {
        return impl::storage_ops<0, Types...>::apply(_tag, storage.data(), v);
    }

    template<typename visitor>
    typename visitor::result_type visit(const visitor& v)const {
        return impl::storage_ops<0, Types...>::apply(_tag, storage.data(), v);
    }

    static int64_t count() { return static_cast< int64_t >( impl::type_info<Types...>::count ); }
    void set_which( int64_t w ) {
      FC_ASSERT( w < count() && w >= 0 );
      clean();
      init_from_tag(w);
    }

    int64_t which() const {return _tag;}
};

template<typename Result>
struct visitor {
    typedef Result result_type;
};

   struct from_static_variant
   {
      variant& var;
      from_static_variant( variant& dv ):var(dv){}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         to_variant( v, var );
      }
   };

   struct to_static_variant
   {
      const variant& var;
      to_static_variant( const variant& dv ):var(dv){}

      typedef void result_type;
      template<typename T> void operator()( T& v )const
      {
         from_variant( var, v );
      }
   };


   template<typename... T> void to_variant( const fc::static_variant<T...>& s, fc::variant& v )
   {
      variant tmp;
      variants vars(2);
      vars[0] = s.which();
      s.visit( from_static_variant(vars[1]) );
      v = std::move(vars);
   }
   template<typename... T> void from_variant( const fc::variant& v, fc::static_variant<T...>& s )
   {
      auto ar = v.get_array();
      if( ar.size() < 2 ) return;
      s.set_which( static_cast< int64_t >( ar[0].as_uint64() ) );
      s.visit( to_static_variant(ar[1]) );
   }

   template<typename T0, typename... T> struct get_typename<T0, T...>  { static const char* name()   { return typeid(static_variant<T0, T...>).name();   } };


} // namespace fc
