#include <sophiatx/plugins/custom_tokens/custom_tokens_plugin.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_api.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_objects.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api.hpp>

#include <fc/crypto/aes.hpp>

namespace sophiatx { namespace plugins { namespace custom_tokens {

namespace detail {

class custom_tokens_api_impl
{
   public:
   custom_tokens_api_impl(custom_tokens_plugin& plugin) :
         _db( plugin.app()->get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ), _plugin(plugin),
         _json_api(plugin.app()->find_plugin< plugins::json_rpc::json_rpc_plugin >()) {};

   DECLARE_API((get_token)(list_token_assets)(list_token_operations)(list_token_errors))

   std::shared_ptr<database_interface> _db;
   custom_tokens_plugin& _plugin;
   plugins::json_rpc::json_rpc_plugin* _json_api;

private:
   alexandria_api::api_account_object get_account(const account_name_type& account) const;
//   void transfer_tokens(const account_name_type& from, const account_name_type& to, const asset& ammount, const string& memo);
//   void create_error_obj(const transaction_id_type& trx_id, const string& error);
};

alexandria_api::api_account_object custom_tokens_api_impl::get_account(const account_name_type& account) const {
   alexandria_api::get_account_args args {account};
   auto result = _json_api->call_api_method("alexandria_api", "get_account", fc::variant(args), [](fc::variant& v, uint64_t i){ FC_UNUSED(v) FC_UNUSED(i)} );
   FC_ASSERT(result.valid(), "Account does not exist!");
   alexandria_api::get_account_return acc_return;
   fc::from_variant( *result, acc_return );
   FC_ASSERT(acc_return.account.size(), "Account does not exist!");
   return acc_return.account[0];
}

DEFINE_API_IMPL( custom_tokens_api_impl, get_token)
{
   get_token_return result;
   const auto& token_idx = _db->get_index< token_index >().indices().get< by_token_symbol >();
   const auto& token_itr = token_idx.find(asset_symbol_type::from_string(args.token_symbol));
   if(token_itr != token_idx.end())
      result = *token_itr;
   return result;
}

DEFINE_API_IMPL( custom_tokens_api_impl, list_token_assets)
{
   list_token_assets_return result;
   FC_ASSERT(args.count < 1000);
   if(args.search_type == "by_account") {
      account_name_type account = args.start;
      const auto& token_idx = _db->get_index< token_accounts_index >().indices().get< by_accounts >();
      auto token_itr = token_idx.lower_bound(account);
      while( token_itr != token_idx.end() && token_itr->account_name == account && result.size() < args.count) {
         result.push_back(*token_itr);
         token_itr++;
      }
      return result;
   }
   else if(args.search_type == "by_token") {
      const auto symbol = asset_symbol_type::from_string( args.start );
      const auto& token_idx = _db->get_index< token_accounts_index >().indices().get< by_token_symbol >();
      auto token_itr = token_idx.lower_bound(symbol);
      while( token_itr != token_idx.end() && token_itr->token_symbol == symbol && result.size() < args.count) {
         result.push_back(*token_itr);
         token_itr++;
      }
      return result;

   } else {
      FC_ASSERT(false, "Unknown search type");
   }
   return result;
}

DEFINE_API_IMPL( custom_tokens_api_impl, list_token_operations)
{
   list_token_operations_return result;
   FC_ASSERT(args.count < 1000);
   if(args.search_type == "by_account") {
      account_name_type account = args.start;
      const auto& token_idx = _db->get_index< token_accounts_index >().indices().get< by_accounts >();
      auto token_itr = token_idx.lower_bound(account);
      while( token_itr != token_idx.end() && token_itr->account_name == account && result.size() < args.count) {
         result.push_back(*token_itr);
         token_itr++;
      }
      return result;
   }
   else if(args.search_type == "by_token") {
      const auto symbol = asset_symbol_type::from_string( args.start );
      const auto& token_idx = _db->get_index< token_accounts_index >().indices().get< by_token_symbol >();
      auto token_itr = token_idx.lower_bound(symbol);
      while( token_itr != token_idx.end() && token_itr->token_symbol == symbol && result.size() < args.count) {
         result.push_back(*token_itr);
         token_itr++;
      }
      return result;

   } else {
      FC_ASSERT(false, "Unknown search type");
   }
   return result;
}

DEFINE_API_IMPL( custom_tokens_api_impl, list_token_errors)
{
   list_token_errors_return result;
   FC_ASSERT(args.count < 1000);
   if(args.search_type == "by_token") {
      account_name_type account = args.start;
      const auto& token_idx = _db->get_index< token_error_index >().indices().get< by_id >();
      auto token_itr = token_idx.lower_bound(args.start);
      while( token_itr != token_idx.end() && result.size() < args.count) {
         result.push_back(*token_itr);
         token_itr++;
      }
      return result;
   }
   else if(args.search_type == "by_tx_id") {
      const auto& token_idx = _db->get_index< token_error_index >().indices().get< by_tx_id >();
      auto token_itr = token_idx.find(args.start);
      if(token_itr != token_idx.end())
         result.push_back(*token_itr);
      return result;
   } else {
      FC_ASSERT(false, "Unknown search type");
   }
   return result;
}

} // detail

custom_tokens_api::custom_tokens_api(custom_tokens_plugin& plugin): my( new detail::custom_tokens_api_impl(plugin) )
{
   JSON_RPC_REGISTER_API( SOPHIATX_CT_PLUGIN_NAME, plugin.app() );
}

custom_tokens_api::~custom_tokens_api() {}

void custom_tokens_api::api_startup() {
}

DEFINE_READ_APIS( custom_tokens_api, (get_token)(list_token_assets)(list_token_operations)(list_token_errors))

} } } // sophiatx::plugins::custom_tokens
