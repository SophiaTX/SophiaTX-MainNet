#include <sophiatx/plugins/custom_tokens/custom_tokens_plugin.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_api.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_objects.hpp>

namespace sophiatx { namespace plugins { namespace custom_tokens {

namespace detail {

class custom_tokens_api_impl
{
   public:
   custom_tokens_api_impl(custom_tokens_plugin& plugin) :
         _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db()), _plugin(plugin),
         _json_api(appbase::app().find_plugin< plugins::json_rpc::json_rpc_plugin >()) {}

   DECLARE_API_IMPL((get_token)(list_token_balances)(list_token_operations)(list_token_errors))

   std::shared_ptr<database_interface> _db;
   custom_tokens_plugin& _plugin;
   plugins::json_rpc::json_rpc_plugin* _json_api;
};


DEFINE_API_IMPL( custom_tokens_api_impl, get_token)
{
   get_token_return result;
   const auto& token_idx = _db->get_index< token_index >().indices().get< by_token_symbol >();
   const auto& token_itr = token_idx.find(asset_symbol_type::from_string(args.token_symbol));
   if(token_itr != token_idx.end())
      result.token = *token_itr;
   return result;
}

DEFINE_API_IMPL( custom_tokens_api_impl, list_token_balances)
{
   list_token_balances_return result;
   FC_ASSERT(args.count < SOPHIATX_API_SINGLE_QUERY_LIMIT);
   if(args.search_type == "by_account") {
      account_name_type account = args.search_field;
      const auto& token_idx = _db->get_index< token_accounts_index >().indices().get< by_account >();
      auto itr = token_idx.upper_bound( boost::make_tuple( account, args.start ) );
      auto end = token_idx.lower_bound( boost::make_tuple( account, int64_t(itr->account_sequence) + args.count ));
      while( itr != end  && result.token_assets.size() < args.count) {
         --itr;
         result.token_assets[itr->account_sequence] = *itr;
      }
      return result;
   }
   else if(args.search_type == "by_token") {
      const auto symbol = asset_symbol_type::from_string( args.search_field );
      const auto& token_idx = _db->get_index< token_accounts_index >().indices().get< by_token_symbol >();
      auto itr = token_idx.upper_bound( boost::make_tuple( symbol, args.start ) );
      auto end = token_idx.lower_bound( boost::make_tuple( symbol, int64_t(itr->token_sequence) + args.count ));
      while( itr != end  && result.token_assets.size() < args.count) {
         --itr;
         result.token_assets[itr->token_sequence] = *itr;
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
   FC_ASSERT(args.count < SOPHIATX_API_SINGLE_QUERY_LIMIT);
   if(args.search_type == "by_account") {
      account_name_type account = args.search_field;
      const auto& token_idx = _db->get_index< token_operation_index >().indices().get< by_account >();
      auto itr = token_idx.upper_bound( boost::make_tuple( account, args.start ) );
      auto end = token_idx.lower_bound( boost::make_tuple( account, int64_t(itr->account_sequence) + args.count  ) );
      while( itr != end  && result.history.size() < args.count) {
         --itr;
         result.history[itr->account_sequence] = *itr;
      }
      return result;
   }
   else if(args.search_type == "by_token") {
      const auto symbol = asset_symbol_type::from_string( args.search_field );
      const auto& token_idx = _db->get_index< token_operation_index >().indices().get< by_token_symbol >();
      auto itr = token_idx.upper_bound( boost::make_tuple( symbol, args.start ) );
      auto end = token_idx.lower_bound( boost::make_tuple( symbol, int64_t(itr->token_sequence) + args.count  ) );
      while( itr != end  && result.history.size() < args.count) {
         --itr;
         result.history[itr->token_sequence] = *itr;
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
   FC_ASSERT(args.count < SOPHIATX_API_SINGLE_QUERY_LIMIT);
   if(args.search_type == "by_token") {

      const auto symbol = asset_symbol_type::from_string( args.search_field );
      const auto& token_idx = _db->get_index< token_error_index >().indices().get< by_token_symbol >();
      auto itr = token_idx.upper_bound( boost::make_tuple( symbol, args.start ) );
      auto end = token_idx.lower_bound( boost::make_tuple( symbol, int64_t(itr->token_sequence) + args.count  ) );
      while( itr != end  && result.errors.size() < args.count) {
         --itr;
         result.errors[itr->token_sequence] = *itr;
      }
      return result;
   }
   else if(args.search_type == "by_tx_id") {
      const auto& token_idx = _db->get_index< token_error_index >().indices().get< by_tx_id >();
      auto token_itr = token_idx.find(fc::ripemd160::hash(args.search_field));
      if(token_itr != token_idx.end())
         result.errors[0] = *token_itr;
      return result;
   } else {
      FC_ASSERT(false, "Unknown search type");
   }
   return result;
}

} // detail

custom_tokens_api::custom_tokens_api(custom_tokens_plugin& plugin): my( new detail::custom_tokens_api_impl(plugin) )
{
   JSON_RPC_REGISTER_API( SOPHIATX_CT_PLUGIN_NAME);
}

custom_tokens_api::~custom_tokens_api() {}

DEFINE_READ_APIS( custom_tokens_api, (get_token) (list_token_balances) (list_token_operations) (list_token_errors))

} } } // sophiatx::plugins::custom_tokens
