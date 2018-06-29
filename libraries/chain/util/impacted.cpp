#include <sophiatx/protocol/authority.hpp>

#include <sophiatx/chain/util/impacted.hpp>

#include <fc/utility.hpp>

namespace sophiatx { namespace app {

using namespace fc;
using namespace sophiatx::protocol;

// TODO:  Review all of these, especially no-ops
struct get_impacted_account_visitor
{
   flat_set<account_name_type>& _impacted;
   get_impacted_account_visitor( flat_set<account_name_type>& impact ):_impacted( impact ) {}
   typedef void result_type;

   template<typename T>
   void operator()( const T& op )
   {
      op.get_required_posting_authorities( _impacted );
      op.get_required_active_authorities( _impacted );
      op.get_required_owner_authorities( _impacted );
   }

   // ops
   void operator()( const account_create_operation& op )
   {

      auto hash = fc::ripemd160::hash(op.name_seed);
      std::vector<char> bytes(hash.data(), hash.data()+hash.data_size());
      account_name_type new_account_name = fc::to_base58(bytes);
      _impacted.insert( new_account_name );
      _impacted.insert( op.creator );
   }

   void operator()( const transfer_operation& op )
   {
      _impacted.insert( op.from );
      _impacted.insert( op.to );
   }

   void operator()( const escrow_transfer_operation& op )
   {
      _impacted.insert( op.from );
      _impacted.insert( op.to );
      _impacted.insert( op.agent );
   }

   void operator()( const escrow_approve_operation& op )
   {
      _impacted.insert( op.from );
      _impacted.insert( op.to );
      _impacted.insert( op.agent );
   }

   void operator()( const escrow_dispute_operation& op )
   {
      _impacted.insert( op.from );
      _impacted.insert( op.to );
      _impacted.insert( op.agent );
   }

   void operator()( const escrow_release_operation& op )
   {
      _impacted.insert( op.from );
      _impacted.insert( op.to );
      _impacted.insert( op.agent );
   }

   void operator()( const transfer_to_vesting_operation& op )
   {
      _impacted.insert( op.from );

      if ( op.to != account_name_type() && op.to != op.from )
      {
         _impacted.insert( op.to );
      }
   }

   void operator()( const account_witness_vote_operation& op )
   {
      _impacted.insert( op.account );
      _impacted.insert( op.witness );
   }

   void operator()( const account_witness_proxy_operation& op )
   {
      _impacted.insert( op.account );
      _impacted.insert( op.proxy );
   }

   void operator()( const feed_publish_operation& op )
   {
      _impacted.insert( op.publisher );
   }

   void operator()( const request_account_recovery_operation& op )
   {
      _impacted.insert( op.account_to_recover );
      _impacted.insert( op.recovery_account );
   }

   void operator()( const recover_account_operation& op )
   {
      _impacted.insert( op.account_to_recover );
   }

   void operator()( const change_recovery_account_operation& op )
   {
      _impacted.insert( op.account_to_recover );
   }

   void operator()( const interest_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const fill_vesting_withdraw_operation& op )
   {
      _impacted.insert( op.from_account );
      _impacted.insert( op.to_account );
   }

   void operator()( const shutdown_witness_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const producer_reward_operation& op )
   {
      _impacted.insert( op.producer );
   }

   void operator()( const promotion_pool_withdraw_operation& op )
   {
      _impacted.insert( op.to_account );
   }

   void operator()( const witness_update_operation& op)
   {
      _impacted.insert( op.owner );
   }

   void operator()( const witness_stop_operation& op)
   {
      _impacted.insert( op.owner );
   }

   void operator()( const sponsor_fees_operation& op)
   {
      if(op.sponsor!="")
         _impacted.insert( op.sponsor );
      _impacted.insert( op.sponsored );
   }

   void operator() (const transfer_from_promotion_pool_operation& op)
   {
      _impacted.insert(SOPHIATX_INIT_MINER_NAME);
      _impacted.insert(op.transfer_to);
   }

   void operator() (const application_create_operation& op)
   {
      _impacted.insert(op.author);
   }

   void operator() (const application_update_operation& op)
   {
      _impacted.insert(op.author);
   }

   void operator() (const application_delete_operation& op)
   {
      _impacted.insert(op.author);
   }
   //void operator()( const operation& op ){}
};

void operation_get_impacted_accounts( const operation& op, flat_set<account_name_type>& result )
{
   get_impacted_account_visitor vtor = get_impacted_account_visitor( result );
   op.visit( vtor );
}

void transaction_get_impacted_accounts( const transaction& tx, flat_set<account_name_type>& result )
{
   for( const auto& op : tx.operations )
      operation_get_impacted_accounts( op, result );
}

} }
