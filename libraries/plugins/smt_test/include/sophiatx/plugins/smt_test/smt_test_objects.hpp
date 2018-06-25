#pragma once
#include <sophiatx/chain/sophiatx_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace sophiatx { namespace plugins { namespace smt_test {

using namespace std;
using namespace sophiatx::chain;

#ifndef SOPHIATX_SMT_TEST_SPACE_ID
#define SOPHIATX_SMT_TEST_SPACE_ID 13
#endif

enum smt_test_object_types
{
   smt_token_object_type = ( SOPHIATX_SMT_TEST_SPACE_ID << 8 )
};

class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   public:
      template< typename Constructor, typename Allocator >
      smt_token_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type                 id;

      account_name_type       control_account;
      uint8_t                 decimal_places = 0;
      int64_t                 max_supply = SOPHIATX_MAX_SHARE_SUPPLY;

      time_point_sec          generation_begin_time;
      time_point_sec          generation_end_time;
      time_point_sec          announced_launch_time;
      time_point_sec          launch_expiration_time;
};

typedef smt_token_object::id_type smt_token_id_type;

using namespace boost::multi_index;

struct by_control_account;

typedef multi_index_container<
   smt_token_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< smt_token_object, smt_token_id_type, &smt_token_object::id > >,
      ordered_unique< tag< by_control_account >,
         composite_key< smt_token_object,
            member< smt_token_object, account_name_type, &smt_token_object::control_account >
         >
      >
   >,
   allocator< smt_token_object >
> smt_token_index;

} } } // sophiatx::plugins::smt_test

FC_REFLECT( sophiatx::plugins::smt_test::smt_token_object,
   (id)
   (control_account)
   (decimal_places)
   (max_supply)
   (generation_begin_time)
   (generation_end_time)
   (announced_launch_time)
   (launch_expiration_time)
   )
CHAINBASE_SET_INDEX_TYPE( sophiatx::plugins::smt_test::smt_token_object, sophiatx::plugins::smt_test::smt_token_index )
