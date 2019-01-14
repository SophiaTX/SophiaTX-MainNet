#ifndef SOPHIATX_REMOTE_DB_API_HPP
#define SOPHIATX_REMOTE_DB_API_HPP

#include <sophiatx/chain/sophiatx_objects.hpp>
#include <sophiatx/chain/account_object.hpp>
#include <sophiatx/chain/block_summary_object.hpp>
#include <sophiatx/chain/global_property_object.hpp>
#include <sophiatx/chain/hardfork_property_object.hpp>
#include <sophiatx/chain/history_object.hpp>
#include <sophiatx/chain/sophiatx_objects.hpp>
#include <sophiatx/chain/witness_objects.hpp>

#include <sophiatx/protocol/block.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>


namespace sophiatx { namespace chain {

using namespace fc;
using namespace std;

/**
 * get_block
 */
// optional<signed_block> get_block(uint32_t num);
struct get_block_args {
   uint32_t	num;
};

struct get_block_return {
   optional<protocol::signed_block>	block;
};

/**
 * This is a dummy API so that the wallet can create properly formatted API calls
 */
struct remote_db_api
{
   get_block_return get_block( chain::get_block_args args );
};


} }

FC_API( sophiatx::chain::remote_db_api,
              (get_block)
)
/**
 * get_block
 */
FC_REFLECT( sophiatx::chain::get_block_args,
            (num) )
FC_REFLECT( sophiatx::chain::get_block_return,
            (block) )

#endif //SOPHIATX_REMOTE_DB_API_HPP
