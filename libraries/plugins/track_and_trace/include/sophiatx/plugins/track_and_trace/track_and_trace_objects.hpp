#pragma once
#include <sophiatx/chain/sophiatx_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

using namespace std;
using namespace sophiatx::chain;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various track_and_trace automagic depends on them being known at compile
// time.
//
#ifndef SOPHIATX_TRACK_AND_TRACE_SPACE_ID
#define SOPHIATX_TRACK_AND_TRACE_SPACE_ID 101
#endif

enum posession_object_types
{
   posession_object_type = ( SOPHIATX_TRACK_AND_TRACE_SPACE_ID << 8 ),
   transfer_history_object_type = ( SOPHIATX_TRACK_AND_TRACE_SPACE_ID << 8 )+1
};

typedef account_name_type tracked_object_name_type;
class possession_object : public object< posession_object_type, possession_object >
{
   public:
      template< typename Constructor, typename Allocator >
      possession_object( Constructor&& c, allocator< Allocator > a ): meta(a), info(a), claim_key(a)
      {
         c( *this );
      }

      id_type           id;

      account_name_type   holder;
      account_name_type   new_holder;
      tracked_object_name_type   serial;
      shared_string     meta;
      shared_string     info;
      shared_string     claim_key;

};

class transfer_history_object : public object< transfer_history_object_type, transfer_history_object >
{
   public:
      template< typename Constructor, typename Allocator >
      transfer_history_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type           id;
      account_name_type          new_holder;
      fc::time_point_sec          change_date;
      tracked_object_name_type   serial;
};

typedef possession_object::id_type possession_object_id_type;
typedef transfer_history_object::id_type transfer_history_object_id_type;


using namespace boost::multi_index;

struct by_holder;
struct by_serial;
struct by_serial_date;
struct by_new_holder;

typedef multi_index_container<
      possession_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< possession_object, possession_object_id_type, &possession_object::id > >,
      ordered_non_unique< tag <by_holder>, member < possession_object, account_name_type, &possession_object::holder > >,
      ordered_unique< tag <by_serial>, member < possession_object, tracked_object_name_type, &possession_object::serial > >,
      ordered_non_unique< tag <by_new_holder>, member < possession_object, account_name_type, &possession_object::new_holder > >
   >,
   allocator< possession_object >
> posession_index;

typedef multi_index_container<
      transfer_history_object,
      indexed_by<
            ordered_unique< tag< by_id >, member< transfer_history_object, transfer_history_object_id_type, &transfer_history_object::id > >,
            ordered_unique< tag <by_serial_date>,
                  composite_key< transfer_history_object,
                        member < transfer_history_object, tracked_object_name_type, &transfer_history_object::serial  >,
                        member < transfer_history_object, fc::time_point_sec, &transfer_history_object::change_date  >
                  >
            >
      >,
      allocator< transfer_history_object >
> transfer_history_index;


} } } // sophiatx::plugins::track_and_trace


FC_REFLECT( sophiatx::plugins::track_and_trace_plugin::possession_object, (id)(serial)(holder)(new_holder)(meta)(info)(claim_key) )
FC_REFLECT( sophiatx::plugins::track_and_trace_plugin::transfer_history_object, (id)(new_holder)(change_date)(serial) )

CHAINBASE_SET_INDEX_TYPE( sophiatx::plugins::track_and_trace_plugin::possession_object, sophiatx::plugins::track_and_trace_plugin::posession_index )
CHAINBASE_SET_INDEX_TYPE( sophiatx::plugins::track_and_trace_plugin::transfer_history_object, sophiatx::plugins::track_and_trace_plugin::transfer_history_index )
