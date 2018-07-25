#pragma once
#include <sophiatx/plugins/block_api/block_api_objects.hpp>

#include <sophiatx/protocol/types.hpp>
#include <sophiatx/protocol/transaction.hpp>
#include <sophiatx/protocol/block_header.hpp>

#include <sophiatx/plugins/json_rpc/utility.hpp>

namespace sophiatx { namespace plugins { namespace block_api {

/* get_block_header */

struct get_block_header_args
{
   uint32_t block_num;
};

struct get_block_header_return
{
   optional< block_header > header;
};

/* get_block */
struct get_block_args
{
   uint32_t block_num;
};

struct get_block_return
{
   optional< api_signed_block_object > block;
};

} } } // sophiatx::block_api

FC_REFLECT( sophiatx::plugins::block_api::get_block_header_args,
   (block_num) )

FC_REFLECT( sophiatx::plugins::block_api::get_block_header_return,
   (header) )

FC_REFLECT( sophiatx::plugins::block_api::get_block_args,
   (block_num) )

FC_REFLECT( sophiatx::plugins::block_api::get_block_return,
   (block) )

