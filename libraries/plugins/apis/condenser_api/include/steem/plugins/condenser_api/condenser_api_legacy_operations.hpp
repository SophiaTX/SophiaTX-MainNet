#include <steem/protocol/operations.hpp>
#include <steem/plugins/condenser_api/condenser_api_legacy_asset.hpp>

namespace steem { namespace plugins { namespace condenser_api {

   using namespace steem::protocol;

   typedef account_update_operation               legacy_account_update_operation;
   typedef placeholder_a_operation                legacy_placeholder_a_operation;
   typedef placeholder_b_operation                legacy_placeholder_b_operation;
   typedef escrow_approve_operation               legacy_escrow_approve_operation;
   typedef escrow_dispute_operation               legacy_escrow_dispute_operation;
   typedef witness_set_properties_operation       legacy_witness_set_properties_operation;
   typedef account_witness_vote_operation         legacy_account_witness_vote_operation;
   typedef account_witness_proxy_operation        legacy_account_witness_proxy_operation;
   typedef custom_operation                       legacy_custom_operation;
   typedef custom_json_operation                  legacy_custom_json_operation;
   typedef custom_binary_operation                legacy_custom_binary_operation;
   typedef report_over_production_operation       legacy_report_over_production_operation;
   typedef request_account_recovery_operation     legacy_request_account_recovery_operation;
   typedef recover_account_operation              legacy_recover_account_operation;
   typedef reset_account_operation                legacy_reset_account_operation;
   typedef set_reset_account_operation            legacy_set_reset_account_operation;
   typedef change_recovery_account_operation      legacy_change_recovery_account_operation;
   typedef shutdown_witness_operation             legacy_shutdown_witness_operation;
   typedef hardfork_operation                     legacy_hardfork_operation;

   struct legacy_price
   {
      legacy_price() {}
      legacy_price( const protocol::price& p ) :
         base( legacy_asset::from_asset( p.base ) ),
         quote( legacy_asset::from_asset( p.quote ) )
      {}

      operator price()const { return price( base, quote ); }

      legacy_asset base;
      legacy_asset quote;
   };

   struct legacy_account_create_operation
   {
      legacy_account_create_operation() {}
      legacy_account_create_operation( const account_create_operation& op ) :
         fee( legacy_asset::from_asset( op.fee ) ),
         creator( op.creator ),
         new_account_name( op.new_account_name ),
         owner( op.owner ),
         active( op.active ),
         memo_key( op.memo_key ),
         json_metadata( op.json_metadata )
      {}

      operator account_create_operation()const
      {
         account_create_operation op;
         op.fee = fee;
         op.creator = creator;
         op.new_account_name = new_account_name;
         op.owner = owner;
         op.active = active;
         op.memo_key = memo_key;
         op.json_metadata = json_metadata;
         return op;
      }

      legacy_asset      fee;
      account_name_type creator;
      account_name_type new_account_name;
      authority         owner;
      authority         active;
      authority         posting;
      public_key_type   memo_key;
      string            json_metadata;
   };

   struct legacy_transfer_operation
   {
      legacy_transfer_operation() {}
      legacy_transfer_operation( const transfer_operation& op ) :
         from( op.from ),
         to( op.to ),
         amount( legacy_asset::from_asset( op.amount ) ),
         memo( op.memo )
      {}

      operator transfer_operation()const
      {
         transfer_operation op;
         op.from = from;
         op.to = to;
         op.amount = amount;
         op.memo = memo;
         return op;
      }

      account_name_type from;
      account_name_type to;
      legacy_asset      amount;
      string            memo;
   };

   struct legacy_escrow_transfer_operation
   {
      legacy_escrow_transfer_operation() {}
      legacy_escrow_transfer_operation( const escrow_transfer_operation& op ) :
         from( op.from ),
         to( op.to ),
         agent( op.agent ),
         escrow_id( op.escrow_id ),
         fee( legacy_asset::from_asset( op.fee ) ),
         ratification_deadline( op.ratification_deadline ),
         escrow_expiration( op.escrow_expiration ),
         json_meta( op.json_meta )
      {}

      operator escrow_transfer_operation()const
      {
         escrow_transfer_operation op;
         op.from = from;
         op.to = to;
         op.agent = agent;
         op.escrow_id = escrow_id;
         op.steem_amount = steem_amount;
         op.fee = fee;
         op.ratification_deadline = ratification_deadline;
         op.escrow_expiration = escrow_expiration;
         op.json_meta = json_meta;
         return op;
      }

      account_name_type from;
      account_name_type to;
      account_name_type agent;
      uint32_t          escrow_id;

      legacy_asset      steem_amount;
      legacy_asset      fee;

      time_point_sec    ratification_deadline;
      time_point_sec    escrow_expiration;

      string            json_meta;
   };

   struct legacy_escrow_release_operation
   {
      legacy_escrow_release_operation() {}
      legacy_escrow_release_operation( const escrow_release_operation& op ) :
         from( op.from ),
         to( op.to ),
         agent( op.agent ),
         who( op.who ),
         receiver( op.receiver ),
         escrow_id( op.escrow_id ),
         steem_amount( legacy_asset::from_asset( op.steem_amount ) )
      {}

      operator escrow_release_operation()const
      {
         escrow_release_operation op;
         op.from = from;
         op.to = to;
         op.agent = agent;
         op.who = who;
         op.receiver = receiver;
         op.escrow_id = escrow_id;
         op.steem_amount = steem_amount;
         return op;
      }

      account_name_type from;
      account_name_type to;
      account_name_type agent;
      account_name_type who;
      account_name_type receiver;

      uint32_t          escrow_id;
      legacy_asset      steem_amount;
   };

   struct legacy_transfer_to_vesting_operation
   {
      legacy_transfer_to_vesting_operation() {}
      legacy_transfer_to_vesting_operation( const transfer_to_vesting_operation& op ) :
         from( op.from ),
         to( op.to ),
         amount( legacy_asset::from_asset( op.amount ) )
      {}

      operator transfer_to_vesting_operation()const
      {
         transfer_to_vesting_operation op;
         op.from = from;
         op.to = to;
         op.amount = amount;
         return op;
      }

      account_name_type from;
      account_name_type to;
      legacy_asset      amount;
   };

   struct legacy_withdraw_vesting_operation
   {
      legacy_withdraw_vesting_operation() {}
      legacy_withdraw_vesting_operation( const withdraw_vesting_operation& op ) :
         account( op.account ),
         vesting_shares( legacy_asset::from_asset( op.vesting_shares) )
      {}

      operator withdraw_vesting_operation()const
      {
         withdraw_vesting_operation op;
         op.account = account;
         op.vesting_shares = vesting_shares;
         return op;
      }

      account_name_type account;
      legacy_asset      vesting_shares;
   };

   struct legacy_witness_update_operation
   {
      legacy_witness_update_operation() {}
      legacy_witness_update_operation( const witness_update_operation& op ) :
         owner( op.owner ),
         url( op.url ),
         block_signing_key( op.block_signing_key ),
         fee( legacy_asset::from_asset( op.fee ) )
      {
         props.account_creation_fee = op.props.account_creation_fee;
         props.maximum_block_size = op.props.maximum_block_size;
      }

      operator witness_update_operation()const
      {
         witness_update_operation op;
         op.owner = owner;
         op.url = url;
         op.block_signing_key = block_signing_key;
         op.props.account_creation_fee = props.account_creation_fee;
         op.props.maximum_block_size = props.maximum_block_size;
         op.fee = fee;
         return op;
      }

      account_name_type       owner;
      string                  url;
      public_key_type         block_signing_key;
      legacy_chain_properties props;
      legacy_asset            fee;
   };

   struct legacy_feed_publish_operation
   {
      legacy_feed_publish_operation() {}
      legacy_feed_publish_operation( const feed_publish_operation& op ) :
         publisher( op.publisher ),
         exchange_rate( legacy_price( op.exchange_rate ) )
      {}

      operator feed_publish_operation()const
      {
         feed_publish_operation op;
         op.publisher = publisher;
         op.exchange_rate = exchange_rate;
         return op;
      }

      account_name_type publisher;
      legacy_price      exchange_rate;
   };

   struct legacy_interest_operation
   {
      legacy_interest_operation() {}
      legacy_interest_operation( const interest_operation& op ) :
         owner( op.owner ),
         interest( legacy_asset::from_asset( op.interest ) )
      {}

      operator interest_operation()const
      {
         interest_operation op;
         op.owner = owner;
         op.interest = interest;
         return op;
      }

      account_name_type owner;
      legacy_asset      interest;
   };

   struct legacy_fill_vesting_withdraw_operation
   {
      legacy_fill_vesting_withdraw_operation() {}
      legacy_fill_vesting_withdraw_operation( const fill_vesting_withdraw_operation& op ) :
         from_account( op.from_account ),
         to_account( op.to_account ),
         withdrawn( legacy_asset::from_asset( op.withdrawn ) ),
         deposited( legacy_asset::from_asset( op.deposited ) )
      {}

      operator fill_vesting_withdraw_operation()const
      {
         fill_vesting_withdraw_operation op;
         op.from_account = from_account;
         op.to_account = to_account;
         op.withdrawn = withdrawn;
         op.deposited = deposited;
         return op;
      }

      account_name_type from_account;
      account_name_type to_account;
      legacy_asset      withdrawn;
      legacy_asset      deposited;
   };

   struct legacy_producer_reward_operation
   {
      legacy_producer_reward_operation() {}
      legacy_producer_reward_operation( const producer_reward_operation& op ) :
         producer( op.producer ),
         vesting_shares( legacy_asset::from_asset( op.vesting_shares ) )
      {}

      operator producer_reward_operation()const
      {
         producer_reward_operation op;
         op.producer = producer;
         op.vesting_shares = vesting_shares;
         return op;
      }

      account_name_type producer;
      legacy_asset      vesting_shares;
   };

   struct legacy_promotion_pool_withdraw_operation
   {
      legacy_promotion_pool_withdraw_operation() {}
      legacy_promotion_pool_withdraw_operation( const promotion_pool_withdraw_operation& op): to_account(op.to_account), withdrawn(legacy_asset::from_asset(op.withdrawn)) {}

      operator promotion_pool_withdraw_operation() const
      {
         promotion_pool_withdraw_operation op;
         op.to_account = to_account;
         op.withdrawn = withdrawn;
      }

      account_name_type to_account;
      legacy_asset      withdrawn;

   };

   typedef fc::static_variant<
            legacy_transfer_operation,
            legacy_transfer_to_vesting_operation,
            legacy_withdraw_vesting_operation,
            legacy_feed_publish_operation,
            legacy_account_create_operation,
            legacy_account_update_operation,
            legacy_witness_update_operation,
            legacy_account_witness_vote_operation,
            legacy_account_witness_proxy_operation,
            legacy_custom_operation,
            legacy_report_over_production_operation,
            legacy_custom_json_operation,
            legacy_placeholder_a_operation,
            legacy_placeholder_b_operation,
            legacy_request_account_recovery_operation,
            legacy_recover_account_operation,
            legacy_change_recovery_account_operation,
            legacy_escrow_transfer_operation,
            legacy_escrow_dispute_operation,
            legacy_escrow_release_operation,
            legacy_escrow_approve_operation,
            legacy_custom_binary_operation,
            legacy_reset_account_operation,
            legacy_set_reset_account_operation,
            legacy_witness_set_properties_operation,
            legacy_interest_operation,
            legacy_fill_vesting_withdraw_operation,
            legacy_shutdown_witness_operation,
            legacy_hardfork_operation,
            legacy_producer_reward_operation,
            legacy_promotion_pool_withdraw_operation
         > legacy_operation;

   struct legacy_operation_conversion_visitor
   {
      legacy_operation_conversion_visitor( legacy_operation& legacy_op ) : l_op( legacy_op ) {}

      typedef bool result_type;

      legacy_operation& l_op;

      bool operator()( const account_update_operation& op )const                 { l_op = op; return true; }
      bool operator()( const placeholder_a_operation& op )const                  { l_op = op; return true; }
      bool operator()( const placeholder_b_operation& op )const                  { l_op = op; return true; }
      bool operator()( const escrow_approve_operation& op )const                 { l_op = op; return true; }
      bool operator()( const escrow_dispute_operation& op )const                 { l_op = op; return true; }
      bool operator()( const witness_set_properties_operation& op )const         { l_op = op; return true; }
      bool operator()( const account_witness_vote_operation& op )const           { l_op = op; return true; }
      bool operator()( const account_witness_proxy_operation& op )const          { l_op = op; return true; }
      bool operator()( const custom_operation& op )const                         { l_op = op; return true; }
      bool operator()( const custom_json_operation& op )const                    { l_op = op; return true; }
      bool operator()( const custom_binary_operation& op )const                  { l_op = op; return true; }
      bool operator()( const report_over_production_operation& op )const         { l_op = op; return true; }
      bool operator()( const request_account_recovery_operation& op )const       { l_op = op; return true; }
      bool operator()( const recover_account_operation& op )const                { l_op = op; return true; }
      bool operator()( const reset_account_operation& op )const                  { l_op = op; return true; }
      bool operator()( const set_reset_account_operation& op )const              { l_op = op; return true; }
      bool operator()( const change_recovery_account_operation& op )const        { l_op = op; return true; }
      bool operator()( const shutdown_witness_operation& op )const               { l_op = op; return true; }
      bool operator()( const hardfork_operation& op )const                       { l_op = op; return true; }

      bool operator()( const transfer_operation& op )const
      {
         l_op = legacy_transfer_operation( op );
         return true;
      }

      bool operator()( const transfer_to_vesting_operation& op )const
      {
         l_op = legacy_transfer_to_vesting_operation( op );
         return true;
      }

      bool operator()( const withdraw_vesting_operation& op )const
      {
         l_op = legacy_withdraw_vesting_operation( op );
         return true;
      }

      bool operator()( const feed_publish_operation& op )const
      {
         l_op = legacy_feed_publish_operation( op );
         return true;
      }

      bool operator()( const account_create_operation& op )const
      {
         l_op = legacy_account_create_operation( op );
         return true;
      }

      bool operator()( const witness_update_operation& op )const
      {
         l_op = legacy_witness_update_operation( op );
         return true;
      }

      bool operator()( const escrow_transfer_operation& op )const
      {
         l_op = legacy_escrow_transfer_operation( op );
         return true;
      }

      bool operator()( const escrow_release_operation& op )const
      {
         l_op = legacy_escrow_release_operation( op );
         return true;
      }

      bool operator()( const interest_operation& op )const
      {
         l_op = legacy_interest_operation( op );
         return true;
      }

      bool operator()( const fill_vesting_withdraw_operation& op )const
      {
         l_op = legacy_fill_vesting_withdraw_operation( op );
         return true;
      }

      bool operator()( const producer_reward_operation& op )const
      {
         l_op = legacy_producer_reward_operation( op );
         return true;
      }

      bool operator()( const promotion_pool_withdraw_operation& op) const
      {
         l_op = legacy_promotion_pool_withdraw_operation(op);
         return true;
      }

      // Should only be SMT ops
      template< typename T >
      bool operator()( const T& )const { return false; }
};

struct convert_from_legacy_operation_visitor
{
   convert_from_legacy_operation_visitor() {}

   typedef operation result_type;

   operation operator()( const legacy_transfer_operation& op )const
   {
      return operation( transfer_operation( op ) );
   }

   operation operator()( const legacy_transfer_to_vesting_operation& op )const
   {
      return operation( transfer_to_vesting_operation( op ) );
   }

   operation operator()( const legacy_withdraw_vesting_operation& op )const
   {
      return operation( withdraw_vesting_operation( op ) );
   }

   operation operator()( const legacy_feed_publish_operation& op )const
   {
      return operation( feed_publish_operation( op ) );
   }

   operation operator()( const legacy_account_create_operation& op )const
   {
      return operation( account_create_operation( op ) );
   }

   operation operator()( const legacy_witness_update_operation& op )const
   {
      return operation( witness_update_operation( op ) );
   }

   operation operator()( const legacy_escrow_transfer_operation& op )const
   {
      return operation( escrow_transfer_operation( op ) );
   }

   operation operator()( const legacy_escrow_release_operation& op )const
   {
      return operation( escrow_release_operation( op ) );
   }

   operation operator()( const legacy_interest_operation& op )const
   {
      return operation( interest_operation( op ) );
   }

   operation operator()( const legacy_fill_vesting_withdraw_operation& op )const
   {
      return operation( fill_vesting_withdraw_operation( op ) );
   }

   operation operator()( const legacy_producer_reward_operation& op )const
   {
      return operation( producer_reward_operation( op ) );
   }

   operation operator()( const legacy_promotion_pool_withdraw_operation& op)const
   {
      return operation( promotion_pool_withdraw_operation(op) );
   }

   template< typename T >
   operation operator()( const T& t )const
   {
      return operation( t );
   }
};

} } } // steem::plugins::condenser_api

namespace fc {

void to_variant( const steem::plugins::condenser_api::legacy_operation&, fc::variant& );
void from_variant( const fc::variant&, steem::plugins::condenser_api::legacy_operation& );

}

FC_REFLECT( steem::plugins::condenser_api::legacy_price, (base)(quote) )
FC_REFLECT( steem::plugins::condenser_api::legacy_feed_publish_operation, (publisher)(exchange_rate) )

FC_REFLECT( steem::plugins::condenser_api::legacy_account_create_operation,
            (fee)
            (creator)
            (new_account_name)
            (owner)
            (active)
            (memo_key)
            (json_metadata) )

FC_REFLECT( steem::plugins::condenser_api::legacy_transfer_operation, (from)(to)(amount)(memo) )
FC_REFLECT( steem::plugins::condenser_api::legacy_transfer_to_vesting_operation, (from)(to)(amount) )
FC_REFLECT( steem::plugins::condenser_api::legacy_withdraw_vesting_operation, (account)(vesting_shares) )
FC_REFLECT( steem::plugins::condenser_api::legacy_witness_update_operation, (owner)(url)(block_signing_key)(props)(fee) )FC_REFLECT( steem::plugins::condenser_api::legacy_escrow_transfer_operation, (from)(to)(steem_amount)(escrow_id)(agent)(fee)(json_meta)(ratification_deadline)(escrow_expiration) );
FC_REFLECT( steem::plugins::condenser_api::legacy_escrow_release_operation, (from)(to)(agent)(who)(receiver)(escrow_id)(steem_amount) );
FC_REFLECT( steem::plugins::condenser_api::legacy_interest_operation, (owner)(interest) )
FC_REFLECT( steem::plugins::condenser_api::legacy_fill_vesting_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited) )
FC_REFLECT( steem::plugins::condenser_api::legacy_producer_reward_operation, (producer)(vesting_shares) )
FC_REFLECT( steem::plugins::condenser_api::legacy_promotion_pool_withdraw_operation, (to_account)(withdrawn) )

FC_REFLECT_TYPENAME( steem::plugins::condenser_api::legacy_operation )
