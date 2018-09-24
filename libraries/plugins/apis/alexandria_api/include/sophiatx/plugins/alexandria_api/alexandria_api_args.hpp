#pragma once

#include <sophiatx/plugins/block_api/block_api.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;

DEFINE_API_ARGS( get_block, block_api::get_block_args, block_api::get_block_return )

} } } // sophiatx::alexandria_api

