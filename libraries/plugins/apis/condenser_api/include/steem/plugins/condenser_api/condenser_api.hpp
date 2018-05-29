#pragma once

#include <steem/plugins/database_api/database_api.hpp>
#include <steem/plugins/block_api/block_api.hpp>
#include <steem/plugins/account_history_api/account_history_api.hpp>
#include <steem/plugins/account_by_key_api/account_by_key_api.hpp>
#include <steem/plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <steem/plugins/witness_api/witness_api.hpp>
#include <steem/plugins/custom_api/custom_api.hpp>

#include <steem/plugins/condenser_api/condenser_api_legacy_objects.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>
#include <fc/api.hpp>

namespace steem { namespace plugins { namespace condenser_api {

using std::vector;
using fc::variant;
using fc::optional;

using namespace chain;

namespace detail{ class condenser_api_impl; }


struct api_operation_object
{
   api_operation_object() {}
   api_operation_object( const account_history::api_operation_object& obj, const legacy_operation& l_op ) :
      trx_id( obj.trx_id ),
      block( obj.block ),
      trx_in_block( obj.trx_in_block ),
      virtual_op( obj.virtual_op ),
      timestamp( obj.timestamp ),
      op( l_op )
   {}

   transaction_id_type  trx_id;
   uint32_t             block = 0;
   uint32_t             trx_in_block = 0;
   uint16_t             op_in_trx = 0;
   uint64_t             virtual_op = 0;
   fc::time_point_sec   timestamp;
   legacy_operation     op;
};

typedef steem::plugins::custom::received_object api_received_object;


struct api_account_object
{
   api_account_object( const database_api::api_account_object& a ) :
      id( a.id ),
      name( a.name ),
      owner( a.owner ),
      active( a.active ),
      memo_key( a.memo_key ),
      json_metadata( a.json_metadata ),
      proxy( a.proxy ),
      last_owner_update( a.last_owner_update ),
      last_account_update( a.last_account_update ),
      created( a.created ),
      recovery_account( a.recovery_account ),
      reset_account( a.reset_account ),
      last_account_recovery( a.last_account_recovery ),
      balance( legacy_asset::from_asset( a.balance ) ),
      vesting_shares( legacy_asset::from_asset( a.vesting_shares ) ),
      vesting_withdraw_rate( legacy_asset::from_asset( a.vesting_withdraw_rate ) ),
      next_vesting_withdrawal( a.next_vesting_withdrawal ),
      withdrawn( a.withdrawn ),
      to_withdraw( a.to_withdraw ),
      witnesses_voted_for( a.witnesses_voted_for )

   {
      proxied_vsf_votes.insert( proxied_vsf_votes.end(), a.proxied_vsf_votes.begin(), a.proxied_vsf_votes.end() );
   }


   api_account_object(){}

   account_id_type   id;

   account_name_type name;
   authority         owner;
   authority         active;
   public_key_type   memo_key;
   string            json_metadata;
   account_name_type proxy;

   time_point_sec    last_owner_update;
   time_point_sec    last_account_update;

   time_point_sec    created;
   account_name_type recovery_account;
   account_name_type reset_account;
   time_point_sec    last_account_recovery;

   legacy_asset      balance;

   legacy_asset      vesting_shares;
   legacy_asset      vesting_withdraw_rate;
   time_point_sec    next_vesting_withdrawal;
   share_type        withdrawn;
   share_type        to_withdraw;

   vector< share_type > proxied_vsf_votes;

   uint16_t          witnesses_voted_for;

};

struct extended_account : public api_account_object
{
   extended_account(){}
   extended_account( const database_api::api_account_object& a ) :
      api_account_object( a ) {}


   legacy_asset                                             vesting_balance;  /// convert vesting_shares to vesting steem
   map< uint64_t, api_operation_object >   transfer_history; /// transfer to/from vesting
   map< uint64_t, api_operation_object >   other_history;
   set< string >                                            witness_votes;
    /// posts recommened for this user
};


struct extended_dynamic_global_properties
{
   extended_dynamic_global_properties() {}
   extended_dynamic_global_properties( const database_api::api_dynamic_global_property_object& o ) :
      head_block_number( o.head_block_number ),
      head_block_id( o.head_block_id ),
      time( o.time ),
      current_witness( o.current_witness ),

      current_supply( legacy_asset::from_asset( o.current_supply ) ),
      total_vesting_shares( legacy_asset::from_asset( o.total_vesting_shares ) ),
      maximum_block_size( o.maximum_block_size ),
      current_aslot( o.current_aslot ),
      recent_slots_filled( o.recent_slots_filled ),
      participation_count( o.participation_count ),
      last_irreversible_block_num( o.last_irreversible_block_num )
   {}

   uint32_t          head_block_number = 0;
   block_id_type     head_block_id;
   time_point_sec    time;
   account_name_type current_witness;

   legacy_asset      current_supply;
   legacy_asset      total_vesting_shares;

   uint32_t          maximum_block_size = 0;
   uint64_t          current_aslot = 0;
   fc::uint128_t     recent_slots_filled;
   uint8_t           participation_count = 0;

   uint32_t          last_irreversible_block_num = 0;

   int32_t           average_block_size = 0;
};

struct legacy_chain_properties
{
   legacy_chain_properties() {}
   legacy_chain_properties( const chain::chain_properties& c ) :
      account_creation_fee( legacy_asset::from_asset( c.account_creation_fee ) ),
      maximum_block_size( c.maximum_block_size )
   {}

   legacy_asset   account_creation_fee;
   uint32_t       maximum_block_size = STEEM_MIN_BLOCK_SIZE_LIMIT * 2;
};

struct api_witness_object
{
   api_witness_object() {}
   api_witness_object( const database_api::api_witness_object& w ) :
      id( w.id ),
      owner( w.owner ),
      created( w.created ),
      url( w.url ),
      total_missed( w.total_missed ),
      last_aslot( w.last_aslot ),
      last_confirmed_block_num( w.last_confirmed_block_num ),
      signing_key( w.signing_key ),
      props( w.props ),
      submitted_exchange_rates( w.submitted_exchange_rates ),
      votes( w.votes ),
      virtual_last_update( w.virtual_last_update ),
      virtual_position( w.virtual_position ),
      virtual_scheduled_time( w.virtual_scheduled_time ),
      running_version( w.running_version ),
      hardfork_version_vote( w.hardfork_version_vote ),
      hardfork_time_vote( w.hardfork_time_vote )
   {}

   witness_id_type  id;
   account_name_type       owner;
   time_point_sec          created;
   string                  url;
   uint32_t                total_missed = 0;
   uint64_t                last_aslot = 0;
   uint64_t                last_confirmed_block_num = 0;
   public_key_type         signing_key;
   legacy_chain_properties props;
   std::map<asset_symbol_type, submitted_exchange_rate> submitted_exchange_rates;

   share_type              votes;
   fc::uint128_t           virtual_last_update;
   fc::uint128_t           virtual_position;
   fc::uint128_t           virtual_scheduled_time = fc::uint128_t::max_value();
   version                 running_version;
   hardfork_version        hardfork_version_vote;
   time_point_sec          hardfork_time_vote = STEEM_GENESIS_TIME;
};

struct api_witness_schedule_object
{
   api_witness_schedule_object() {}
   api_witness_schedule_object( const database_api::api_witness_schedule_object& w ) :
      id( w.id ),
      current_virtual_time( w.current_virtual_time ),
      next_shuffle_block_num( w.next_shuffle_block_num ),
      num_scheduled_witnesses( w.num_scheduled_witnesses ),
      top19_weight( w.top19_weight ),
      timeshare_weight( w.timeshare_weight ),
      witness_pay_normalization_factor( w.witness_pay_normalization_factor ),
      median_props( w.median_props ),
      majority_version( w.majority_version ),
      max_voted_witnesses( w.max_voted_witnesses ),
      max_runner_witnesses( w.max_runner_witnesses ),
      hardfork_required_witnesses( w.hardfork_required_witnesses )
   {
      current_shuffled_witnesses.insert( current_shuffled_witnesses.begin(), w.current_shuffled_witnesses.begin(), w.current_shuffled_witnesses.end() );
   }

   witness_schedule_id_type      id;
   fc::uint128_t                 current_virtual_time;
   uint32_t                      next_shuffle_block_num = 1;
   vector< account_name_type >   current_shuffled_witnesses;
   uint8_t                       num_scheduled_witnesses = 1;
   uint8_t                       top19_weight = 1;
   uint8_t                       timeshare_weight = 5;
   uint32_t                      witness_pay_normalization_factor = 25;
   legacy_chain_properties       median_props;
   version                       majority_version;
   uint8_t                       max_voted_witnesses           = STEEM_MAX_VOTED_WITNESSES_HF0;
   uint8_t                       max_runner_witnesses          = STEEM_MAX_RUNNER_WITNESSES_HF0;
   uint8_t                       hardfork_required_witnesses   = STEEM_HARDFORK_REQUIRED_WITNESSES;
};

struct api_feed_history_object
{
   api_feed_history_object() {}
   api_feed_history_object( const database_api::api_feed_history_object& f ) :
      current_median_history( f.current_median_history )
   {
      for( auto& p : f.price_history )
      {
         price_history.push_back( legacy_price( p ) );
      }
   }

   feed_history_id_type   id;
   legacy_price           current_median_history;
   deque< legacy_price >  price_history;
};



struct api_escrow_object
{
   api_escrow_object() {}
   api_escrow_object( const database_api::api_escrow_object& e ) :
      id( e.id ),
      escrow_id( e.escrow_id ),
      from( e.from ),
      to( e.to ),
      agent( e.agent ),
      ratification_deadline( e.ratification_deadline ),
      escrow_expiration( e.escrow_expiration ),
      steem_balance( legacy_asset::from_asset( e.steem_balance ) ),
      pending_fee( legacy_asset::from_asset( e.pending_fee ) ),
      to_approved( e.to_approved ),
      disputed( e.disputed ),
      agent_approved( e.agent_approved )
   {}

   escrow_id_type    id;
   uint32_t          escrow_id = 20;
   account_name_type from;
   account_name_type to;
   account_name_type agent;
   time_point_sec    ratification_deadline;
   time_point_sec    escrow_expiration;
   legacy_asset      steem_balance;
   legacy_asset      pending_fee;
   bool              to_approved = false;
   bool              disputed = false;
   bool              agent_approved = false;
};

struct api_application_object
{
   api_application_object( const database_api::api_application_object& a ) :
         id( a.id ),
         name( a.name ),
         author( a.author ),
         url( a.url ),
         metadata( a.metadata ),
         price_param( a.price_param )
   {}

   api_application_object() {}

   application_id_type                             id;
   string                                          name;
   account_name_type                               author;
   string                                          url;
   string                                          metadata;
   application_price_param                         price_param;
};

struct state
{
   string                                             current_route;

   extended_dynamic_global_properties                 props;

   map< string, extended_account >                    accounts;

   map< string, api_witness_object >                  witnesses;
   api_witness_schedule_object                        witness_schedule;
   legacy_price                                       feed_price;
   string                                             error;
};

struct scheduled_hardfork
{
   hardfork_version     hf_version;
   fc::time_point_sec   live_time;
};

typedef vector< variant > get_version_args;

struct get_version_return
{
   get_version_return() {}
   get_version_return( fc::string bc_v, fc::string s_v, fc::string fc_v )
      :blockchain_version( bc_v ), steem_revision( s_v ), fc_revision( fc_v ) {}

   fc::string blockchain_version;
   fc::string steem_revision;
   fc::string fc_revision;
};

typedef map< uint32_t, api_operation_object > get_account_history_return_type;
typedef map< uint64_t, api_received_object >      get_received_documents_return_type;


#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;

/*               API,                                    args,                return */
DEFINE_API_ARGS( get_state,                              vector< variant >,   state )
DEFINE_API_ARGS( get_active_witnesses,                   vector< variant >,   vector< account_name_type > )
DEFINE_API_ARGS( get_block_header,                       vector< variant >,   optional< block_header > )
DEFINE_API_ARGS( get_block,                              vector< variant >,   optional< legacy_signed_block > )
DEFINE_API_ARGS( get_ops_in_block,                       vector< variant >,   vector< api_operation_object > )
DEFINE_API_ARGS( get_config,                             vector< variant >,   fc::variant_object )
DEFINE_API_ARGS( get_dynamic_global_properties,          vector< variant >,   extended_dynamic_global_properties )
DEFINE_API_ARGS( get_chain_properties,                   vector< variant >,   legacy_chain_properties )
DEFINE_API_ARGS( get_current_median_history_price,       vector< variant >,   legacy_price )
DEFINE_API_ARGS( get_feed_history,                       vector< variant >,   api_feed_history_object )
DEFINE_API_ARGS( get_witness_schedule,                   vector< variant >,   api_witness_schedule_object )
DEFINE_API_ARGS( get_hardfork_version,                   vector< variant >,   hardfork_version )
DEFINE_API_ARGS( get_next_scheduled_hardfork,            vector< variant >,   scheduled_hardfork )
DEFINE_API_ARGS( get_key_references,                     vector< variant >,   vector< vector< account_name_type > > )
DEFINE_API_ARGS( get_accounts,                           vector< variant >,   vector< extended_account > )
DEFINE_API_ARGS( get_account_references,                 vector< variant >,   vector< account_id_type > )
DEFINE_API_ARGS( lookup_account_names,                   vector< variant >,   vector< optional< api_account_object > > )
DEFINE_API_ARGS( lookup_accounts,                        vector< variant >,   set< string > )
DEFINE_API_ARGS( get_account_count,                      vector< variant >,   uint64_t )
DEFINE_API_ARGS( get_owner_history,                      vector< variant >,   vector< database_api::api_owner_authority_history_object > )
DEFINE_API_ARGS( get_recovery_request,                   vector< variant >,   optional< database_api::api_account_recovery_request_object > )
DEFINE_API_ARGS( get_escrow,                             vector< variant >,   optional< api_escrow_object > )
DEFINE_API_ARGS( get_witnesses,                          vector< variant >,   vector< optional< api_witness_object > > )
DEFINE_API_ARGS( get_witness_by_account,                 vector< variant >,   optional< api_witness_object > )
DEFINE_API_ARGS( get_witnesses_by_vote,                  vector< variant >,   vector< api_witness_object > )
DEFINE_API_ARGS( lookup_witness_accounts,                vector< variant >,   vector< account_name_type > )
DEFINE_API_ARGS( get_witness_count,                      vector< variant >,   uint64_t )
DEFINE_API_ARGS( get_transaction_hex,                    vector< variant >,   string )
DEFINE_API_ARGS( get_transaction,                        vector< variant >,   annotated_signed_transaction )
DEFINE_API_ARGS( get_required_signatures,                vector< variant >,   set< public_key_type > )
DEFINE_API_ARGS( get_potential_signatures,               vector< variant >,   set< public_key_type > )
DEFINE_API_ARGS( verify_authority,                       vector< variant >,   bool )
DEFINE_API_ARGS( verify_account_authority,               vector< variant >,   bool )
DEFINE_API_ARGS( get_account_history,                    vector< variant >,   get_account_history_return_type )
DEFINE_API_ARGS( broadcast_transaction,                  vector< variant >,   json_rpc::void_type )
DEFINE_API_ARGS( broadcast_transaction_synchronous,      vector< variant >,   network_broadcast_api::broadcast_transaction_synchronous_return )
DEFINE_API_ARGS( broadcast_block,                        vector< variant >,   json_rpc::void_type )
DEFINE_API_ARGS( get_applications,                       vector< variant >,   vector< api_application_object > )
DEFINE_API_ARGS( get_promotion_pool_balance,             vector< variant >,   legacy_asset)
DEFINE_API_ARGS( get_received_documents,                           vector< variant >,   get_received_documents_return_type )
#undef DEFINE_API_ARGS

class condenser_api
{
public:
   condenser_api();
   ~condenser_api();

   DECLARE_API(
      (get_version)
      (get_state)
      (get_active_witnesses)
      (get_block_header)
      (get_block)
      (get_ops_in_block)
      (get_config)
      (get_dynamic_global_properties)
      (get_chain_properties)
      (get_current_median_history_price)
      (get_feed_history)
      (get_witness_schedule)
      (get_hardfork_version)
      (get_next_scheduled_hardfork)
      (get_key_references)
      (get_accounts)
      (get_account_references)
      (lookup_account_names)
      (lookup_accounts)
      (get_account_count)
      (get_owner_history)
      (get_recovery_request)
      (get_escrow)
      (get_witnesses)
      (get_witness_by_account)
      (get_witnesses_by_vote)
      (lookup_witness_accounts)
      (get_witness_count)
      (get_transaction_hex)
      (get_transaction)
      (get_required_signatures)
      (get_potential_signatures)
      (verify_authority)
      (verify_account_authority)
      (get_account_history)
      (get_received_documents)
      (broadcast_transaction)
      (broadcast_transaction_synchronous)
      (broadcast_block)
      (get_applications)
      (get_promotion_pool_balance)
   )

   private:
      friend class condenser_api_plugin;
      void api_startup();

      std::unique_ptr< detail::condenser_api_impl > my;
};

} } } // steem::plugins::condenser_api

FC_REFLECT( steem::plugins::condenser_api::state,
            (current_route)(props)(accounts)(witnesses)(witness_schedule)(feed_price)(error) )

FC_REFLECT( steem::plugins::condenser_api::api_operation_object,
             (trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(op) )

FC_REFLECT( steem::plugins::condenser_api::api_account_object,
             (id)(name)(owner)(active)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)
             (recovery_account)(last_account_recovery)(reset_account)
             (balance)
             (vesting_shares)(vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)
             (proxied_vsf_votes)(witnesses_voted_for)
          )

FC_REFLECT_DERIVED( steem::plugins::condenser_api::extended_account, (steem::plugins::condenser_api::api_account_object),
            (vesting_balance)(transfer_history)(other_history)(witness_votes) )

FC_REFLECT( steem::plugins::condenser_api::extended_dynamic_global_properties,
            (head_block_number)(head_block_id)(time)
            (current_witness)
            (current_supply)
            (total_vesting_shares)
            (maximum_block_size)(current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)
            (average_block_size) )

FC_REFLECT( steem::plugins::condenser_api::legacy_chain_properties,
            (account_creation_fee)(maximum_block_size)
          )

FC_REFLECT( steem::plugins::condenser_api::api_witness_object,
             (id)
             (owner)
             (created)
             (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_aslot)(last_confirmed_block_num)(signing_key)
             (props)
             (submitted_exchange_rates)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )

FC_REFLECT( steem::plugins::condenser_api::api_witness_schedule_object,
             (id)
             (current_virtual_time)
             (next_shuffle_block_num)
             (current_shuffled_witnesses)
             (num_scheduled_witnesses)
             (top19_weight)
             (timeshare_weight)
             (witness_pay_normalization_factor)
             (median_props)
             (majority_version)
             (max_voted_witnesses)
             (max_runner_witnesses)
             (hardfork_required_witnesses)
          )

FC_REFLECT( steem::plugins::condenser_api::api_feed_history_object,
             (id)
             (current_median_history)
             (price_history)
          )


FC_REFLECT( steem::plugins::condenser_api::api_escrow_object,
             (id)(escrow_id)(from)(to)(agent)
             (ratification_deadline)(escrow_expiration)
             (steem_balance)(pending_fee)
             (to_approved)(agent_approved)(disputed) )


FC_REFLECT( steem::plugins::condenser_api::scheduled_hardfork,
            (hf_version)(live_time) )

FC_REFLECT( steem::plugins::condenser_api::get_version_return,
            (blockchain_version)(steem_revision)(fc_revision) )

FC_REFLECT( steem::plugins::condenser_api::api_application_object,
            (id)
            (name)
            (author)
            (url)
            (metadata)
            (price_param)
)

