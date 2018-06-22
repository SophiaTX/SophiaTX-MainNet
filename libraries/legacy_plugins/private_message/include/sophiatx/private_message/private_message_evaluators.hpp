#pragma once

#include <sophiatx/chain/evaluator.hpp>

#include <sophiatx/private_message/private_message_operations.hpp>
#include <sophiatx/private_message/private_message_plugin.hpp>

namespace sophiatx { namespace private_message {

SOPHIATX_DEFINE_PLUGIN_EVALUATOR( private_message_plugin, sophiatx::private_message::private_message_plugin_operation, private_message )

} }
