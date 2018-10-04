#include <sophiatx/plugins/alexandria_api/alexandria_api_operations.hpp>

#define API_PREFIX "api_"
#define API_PREFIX_OFFSET (7)

namespace fc {

std::string name_from_api_type( const std::string& type_name )
{
   auto start = type_name.find( API_PREFIX );
   if( start == std::string::npos )
   {
      start = type_name.find_last_of( ':' ) + 1;
   }
   else
   {
      start += API_PREFIX_OFFSET;
   }
   auto end   = type_name.find_last_of( '_' );

   return type_name.substr( start, end-start );
}

struct from_operation
{
   variant& var;
   from_operation( variant& dv )
      : var( dv ) {}

   typedef void result_type;
   template<typename T> void operator()( const T& v )const
   {
      auto name = name_from_api_type( fc::get_typename< T >::name() );
      var = variant( std::make_pair( name, v ) );
   }
};

struct get_operation_name
{
   string& name;
   get_operation_name( string& dv )
      : name( dv ) {}

   typedef void result_type;
   template< typename T > void operator()( const T& v )const
   {
      name = name_from_api_type( fc::get_typename< T >::name() );
   }
};

void to_variant( const sophiatx::plugins::alexandria_api::api_operation& var,  fc::variant& vo )
{
   var.visit( from_operation( vo ) );
}

void from_variant( const fc::variant& var, sophiatx::plugins::alexandria_api::api_operation& vo )
{
   static std::map<string,uint32_t> to_tag = []()
   {
      std::map<string,uint32_t> name_map;
      for( int i = 0; i < sophiatx::plugins::alexandria_api::api_operation::count(); ++i )
      {
         sophiatx::plugins::alexandria_api::api_operation tmp;
         tmp.set_which(i);
         string n;
         tmp.visit( get_operation_name(n) );
         name_map[n] = i;
      }
      return name_map;
   }();

   auto ar = var.get_array();
   if( ar.size() < 2 ) return;
   if( ar[0].is_uint64() )
      vo.set_which( ar[0].as_uint64() );
   else
   {
      auto itr = to_tag.find(ar[0].as_string());
      FC_ASSERT( itr != to_tag.end(), "Invalid operation name: ${n}", ("n", ar[0]) );
      vo.set_which( to_tag[ar[0].as_string()] );
   }
      vo.visit( fc::to_static_variant( ar[1] ) );
}

} // fc
