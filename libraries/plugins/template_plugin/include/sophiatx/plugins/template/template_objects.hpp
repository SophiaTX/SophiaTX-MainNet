#pragma once
#include <sophiatx/chain/sophiatx_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace sophiatx { namespace plugins { namespace template_plugin {

using namespace std;
using namespace sophiatx::chain;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef SOPHIATX_TEMPLATE_SPACE_ID
#define SOPHIATX_TEMPLATE_SPACE_ID 31
#endif

enum template_object_types
{
   template_lookup_object_type = ( SOPHIATX_TEMPLATE_SPACE_ID << 8 )
};

class template_lookup_object : public object< template_lookup_object_type, template_lookup_object >
{
   public:
      template< typename Constructor, typename Allocator >
      template_lookup_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type           id;

};

typedef template_lookup_object::id_type template_lookup_id_type;


using namespace boost::multi_index;

struct by_key;

typedef multi_index_container<
   template_lookup_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< template_lookup_object, template_lookup_id_type, &template_lookup_object::id > >
   >,
   allocator< template_lookup_object >
> template_lookup_index;

} } } // sophiatx::plugins::template


FC_REFLECT( sophiatx::plugins::template_plugin::template_lookup_object, (id) )
CHAINBASE_SET_INDEX_TYPE( sophiatx::plugins::template_plugin::template_lookup_object, sophiatx::plugins::template_plugin::template_lookup_index )
