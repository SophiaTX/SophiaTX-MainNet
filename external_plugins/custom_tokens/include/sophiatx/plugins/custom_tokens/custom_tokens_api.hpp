#pragma once

#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_objects.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_plugin.hpp>

#include <sophiatx/protocol/operations.hpp>
#include <sophiatx/protocol/types.hpp>

#include <optional>
#include <fc/variant.hpp>

namespace sophiatx {
namespace plugins {
namespace custom_tokens {

namespace detail {
class custom_tokens_api_impl;
}

class api_custom_token {
public:
   api_custom_token() {}

   api_custom_token(const custom_token_object &cto) : token_symbol(cto.token_symbol.to_string()),
                                                      owner_name(cto.owner_name), total_supply(cto.total_supply),
                                                      max_supply(cto.max_supply), burned(cto.burned),
                                                      paused(cto.paused) {}

   string token_symbol;
   account_name_type owner_name;
   uint64_t total_supply;
   uint64_t max_supply;
   uint64_t burned;
   bool paused;
};

struct get_token_args {
   string token_symbol;
};

struct get_token_return {
   optional<api_custom_token> token;
};

class api_custom_token_account {
public:
   api_custom_token_account() {}

   api_custom_token_account(const custom_token_account_object &cto) : account_name(cto.account_name),
                                                                      amount(cto.amount, cto.token_symbol) {}

   account_name_type account_name;
   asset amount;
};

struct list_token_balances_args {
   string search_type; //"by_token", "by_account"
   string search_field; //token name, account
   uint64_t start;
   uint32_t count;
};

struct list_token_balances_return {
   std::map<uint64_t, api_custom_token_account> token_assets;
};

struct api_custom_token_operation_object {
   api_custom_token_operation_object() {}

   api_custom_token_operation_object(const custom_token_operation_object &op_obj) :
         trx_id(op_obj.trx_id),
         block(op_obj.block),
         timestamp(op_obj.timestamp) {
      op = fc::raw::unpack_from_buffer<sophiatx::protocol::operation>(op_obj.serialized_op, 0);
   }

   sophiatx::protocol::transaction_id_type trx_id;
   uint32_t block = 0;
   fc::time_point_sec timestamp;
   sophiatx::protocol::operation op;
};

struct list_token_operations_args {
   string search_type; //"by_token", "by_account", "by_tx_id"
   string search_field; //token name, account, tx_id
   uint64_t start;
   uint32_t count;
};

struct list_token_operations_return {
   std::map<uint64_t, api_custom_token_operation_object> history;
};

class api_custom_token_error {
public:
   api_custom_token_error() {}

   api_custom_token_error(const custom_token_error_object &cto) : trx_id(cto.trx_id),
                                                                  token_symbol(cto.token_symbol.to_string()),
                                                                  error(to_string(cto.error)) {}

   transaction_id_type trx_id;
   string token_symbol;
   string error;
};

struct list_token_errors_args {
   string search_type; //"by_index" (start = -1 -> latest), "by_tx_id"
   string search_field; //token symbol, tx_id
   uint64_t start;
   uint32_t count;
};

struct list_token_errors_return {
   std::map<uint64_t, api_custom_token_error> errors;
};

class custom_tokens_api {
public:
   custom_tokens_api(custom_tokens_plugin &plugin);

   ~custom_tokens_api();

   DECLARE_API((get_token)(list_token_balances)(list_token_operations)(list_token_errors))

private:
   std::unique_ptr<detail::custom_tokens_api_impl> my;
};

}
}
} // sophiatx::plugins::custom_tokens


FC_REFLECT(sophiatx::plugins::custom_tokens::api_custom_token,
           (token_symbol)(owner_name)(total_supply)(max_supply)(burned)(paused))
FC_REFLECT(sophiatx::plugins::custom_tokens::get_token_args,
           (token_symbol))
FC_REFLECT(sophiatx::plugins::custom_tokens::get_token_return, (token))


FC_REFLECT(sophiatx::plugins::custom_tokens::api_custom_token_account,
           (account_name)(amount))
FC_REFLECT(sophiatx::plugins::custom_tokens::list_token_balances_args,
           (search_type)(search_field)(start)(count))
FC_REFLECT(sophiatx::plugins::custom_tokens::list_token_balances_return,
           (token_assets))


FC_REFLECT(sophiatx::plugins::custom_tokens::api_custom_token_operation_object,
           (trx_id)(block)(timestamp)(op))
FC_REFLECT(sophiatx::plugins::custom_tokens::list_token_operations_args,
           (search_type)(search_field)(start)(count))
FC_REFLECT(sophiatx::plugins::custom_tokens::list_token_operations_return,
           (history))


FC_REFLECT(sophiatx::plugins::custom_tokens::api_custom_token_error,
           (trx_id)(token_symbol)(error))
FC_REFLECT(sophiatx::plugins::custom_tokens::list_token_errors_args,
           (search_type)(search_field)(start)(count))
FC_REFLECT(sophiatx::plugins::custom_tokens::list_token_errors_return,
           (errors))
