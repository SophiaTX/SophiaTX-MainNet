#include <sophiatx/protocol/operations.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_asset.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

   using namespace sophiatx::protocol;

   typedef account_update_operation               api_account_update_operation;
   typedef account_witness_vote_operation         api_account_witness_vote_operation;
   typedef account_witness_proxy_operation        api_account_witness_proxy_operation;
   typedef custom_json_operation                  api_custom_json_operation;
   typedef custom_binary_operation                api_custom_binary_operation;
   typedef witness_stop_operation                 api_witness_stop_operation;
   typedef account_create_operation               api_account_create_operation;
   typedef account_delete_operation               api_account_delete_operation;
   typedef price                                  api_price;
   typedef transfer_operation                     api_transfer_operation;
   typedef transfer_to_vesting_operation          api_transfer_to_vesting_operation;
   typedef withdraw_vesting_operation             api_withdraw_vesting_operation;
   typedef witness_update_operation               api_witness_update_operation;
   typedef application_create_operation           api_application_create_operation;
   typedef application_update_operation           api_application_update_operation;
   typedef application_delete_operation           api_application_delete_operation;
   typedef sponsor_fees_operation                 api_sponsor_fees_operation;
   typedef buy_application_operation              api_buy_application_operation;
   typedef cancel_application_buying_operation    api_cancel_application_buying_operation;

   typedef fc::static_variant<
            api_transfer_operation,
            api_transfer_to_vesting_operation,
            api_withdraw_vesting_operation,
            api_account_create_operation,
            api_account_update_operation,
            api_account_delete_operation,
            api_witness_update_operation,
            api_witness_stop_operation,
            api_account_witness_vote_operation,
            api_account_witness_proxy_operation,
            api_custom_json_operation,
            api_custom_binary_operation,
            api_application_create_operation,
            api_application_update_operation,
            api_application_delete_operation,
            api_buy_application_operation,
            api_cancel_application_buying_operation,
            api_sponsor_fees_operation
         > api_operation;

   struct api_operation_conversion_visitor
   {
      api_operation_conversion_visitor( api_operation& api_op ) : l_op( api_op ) {}

      typedef bool result_type;

      api_operation& l_op;
      bool operator()( const transfer_operation& op )const                       { l_op = op; return true; }
      bool operator()( const transfer_to_vesting_operation& op )const            { l_op = op; return true; }
      bool operator()( const withdraw_vesting_operation& op )const               { l_op = op; return true; }
      bool operator()( const account_create_operation& op )const                 { l_op = op; return true; }
      bool operator()( const account_update_operation& op )const                 { l_op = op; return true; }
      bool operator()( const account_delete_operation& op )const                 { l_op = op; return true; }
      bool operator()( const witness_update_operation& op )const                 { l_op = op; return true; }
      bool operator()( const witness_stop_operation& op )const                   { l_op = op; return true; }
      bool operator()( const account_witness_vote_operation& op )const           { l_op = op; return true; }
      bool operator()( const account_witness_proxy_operation& op )const          { l_op = op; return true; }
      bool operator()( const custom_json_operation& op )const                    { l_op = op; return true; }
      bool operator()( const custom_binary_operation& op )const                  { l_op = op; return true; }
      bool operator()( const application_create_operation& op )const             { l_op = op; return true; }
      bool operator()( const application_update_operation& op )const             { l_op = op; return true; }
      bool operator()( const application_delete_operation& op )const             { l_op = op; return true; }
      bool operator()( const buy_application_operation& op )const                { l_op = op; return true; }
      bool operator()( const cancel_application_buying_operation& op )const      { l_op = op; return true; }
      bool operator()( const sponsor_fees_operation& op )const                   { l_op = op; return true; }

      // Should only be SMT ops
      template< typename T >
      bool operator()( const T& )const { return false; }
};

struct convert_from_api_operation_visitor
{
   convert_from_api_operation_visitor() {}

   typedef operation result_type;

   template< typename T >
   operation operator()( const T& t )const
   {
      return operation( t );
   }
};

} } } // sophiatx::plugins::alexandria_api

namespace fc {

void to_variant( const sophiatx::plugins::alexandria_api::api_operation&, fc::variant& );
void from_variant( const fc::variant&, sophiatx::plugins::alexandria_api::api_operation& );

}

FC_REFLECT_TYPENAME( sophiatx::plugins::alexandria_api::api_operation )
