#pragma once
#include <sophiatx/plugins/alexandria_api/alexandria_api_operations.hpp>

#include <sophiatx/plugins/block_api/block_api_objects.hpp>
#include <sophiatx/plugins/account_history_api/account_history_objects.hpp>
#include <sophiatx/plugins/database_api/database_api_objects.hpp>
#include <sophiatx/plugins/custom_api/custom_api.hpp>  //TODO: separate custom_api_objects

#include <sophiatx/chain/sophiatx_object_types.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

typedef sophiatx::plugins::custom::received_object api_received_object;
typedef sophiatx::plugins::database_api::api_application_buying_object api_application_buying_object;

struct key_pair_st
{
   public_key_type      pub_key;
   string               wif_priv_key;
};

enum authority_type
{
   owner,
   active
};

struct memo_data {

   static fc::optional<memo_data> from_string( string str ) {
      try {
         if( str.size() > sizeof(memo_data)) {
            auto data = fc::from_base58( str );
            auto m  = fc::raw::unpack_from_vector<memo_data>( data, 0 );
            FC_ASSERT( string(m) == str );
            return m;
         }
      } catch ( ... ) {}
      return fc::optional<memo_data>();
   }

   int64_t         nonce = 0;
   uint64_t        check = 0;
   vector<char>    encrypted;

   operator string()const {
      auto data = fc::raw::pack_to_vector(*this);
      auto base58 = fc::to_base58( data );
      return base58;
   }
};


struct api_signed_transaction
{
   api_signed_transaction() {}
   api_signed_transaction( const annotated_signed_transaction& t ) :
         ref_block_num( t.ref_block_num ),
         ref_block_prefix( t.ref_block_prefix ),
         expiration( t.expiration ),
         transaction_id( t.transaction_id ),
         block_num( t.block_num ),
         transaction_num( t.transaction_num )
   {
      for( const auto& o : t.operations )
      {
         api_operation op;
         o.visit( api_operation_conversion_visitor( op ) );
         operations.push_back( op );
      }

      // Signed transaction extensions field exists, but must be empty
      // Don't worry about copying them.

      signatures.insert( signatures.end(), t.signatures.begin(), t.signatures.end() );
   }

   operator signed_transaction()const
   {
      signed_transaction tx;
      tx.ref_block_num = ref_block_num;
      tx.ref_block_prefix = ref_block_prefix;
      tx.expiration = expiration;

      convert_from_api_operation_visitor v;
      for( const auto& o : operations )
      {
         tx.operations.push_back( o.visit( v ) );
      }

      tx.signatures.insert( tx.signatures.end(), signatures.begin(), signatures.end() );

      return tx;
   }

   uint16_t                   ref_block_num    = 0;
   uint32_t                   ref_block_prefix = 0;
   fc::time_point_sec         expiration;
   vector< api_operation >    operations;
   extensions_type            extensions;
   vector< signature_type >   signatures;
   transaction_id_type        transaction_id;
   uint32_t                   block_num = 0;
   uint32_t                   transaction_num = 0;
};


struct api_signed_block
{
   api_signed_block() {}
   api_signed_block( const block_api::api_signed_block_object& b ) :
         previous( b.previous ),
         timestamp( b.timestamp ),
         witness( b.witness ),
         transaction_merkle_root( b.transaction_merkle_root ),
         witness_signature( b.witness_signature ),
         block_id( b.block_id ),
         signing_key( b.signing_key )
   {
      extensions.insert( b.extensions.begin(), b.extensions.end() );

      for( const auto& t : b.transactions )
      {
         transactions.push_back( api_signed_transaction( t ) );
      }

      transaction_ids.insert( transaction_ids.end(), b.transaction_ids.begin(), b.transaction_ids.end() );
   }

   operator signed_block()const
   {
      signed_block b;
      b.previous = previous;
      b.timestamp = timestamp;
      b.witness = witness;
      b.transaction_merkle_root = transaction_merkle_root;
      b.extensions.insert( extensions.begin(), extensions.end() );
      b.witness_signature = witness_signature;

      for( const auto& t : transactions )
      {
         b.transactions.push_back( signed_transaction( t ) );
      }

      return b;
   }

   block_id_type                       previous;
   fc::time_point_sec                  timestamp;
   string                              witness;
   checksum_type                       transaction_merkle_root;
   block_header_extensions_type        extensions;
   signature_type                      witness_signature;
   vector< api_signed_transaction >    transactions;
   block_id_type                       block_id;
   public_key_type                     signing_key;
   vector< transaction_id_type >       transaction_ids;
};


struct api_feed_history_object
{
   api_feed_history_object() {}
   api_feed_history_object( const database_api::api_feed_history_object& f ) :
         current_median_price( f.current_median_history )
   {
      for( auto& p : f.price_history )
      {
         price_history.push_back( api_price( p ) );
      }
   }

   chain::feed_history_id_type    id;
   api_price                      current_median_price;
   deque< api_price >             price_history;
};


struct api_operation_object
{
   api_operation_object() {}
   api_operation_object( const account_history::api_operation_object& obj, const api_operation& l_op ) :
         trx_id( obj.trx_id ),
         block( obj.block ),
         trx_in_block( obj.trx_in_block ),
         virtual_op( obj.virtual_op ),
         timestamp( obj.timestamp ),
         op( l_op ),
         fee_payer( obj.fee_payer)
   {}

   transaction_id_type  trx_id;
   uint32_t             block = 0;
   uint32_t             trx_in_block = 0;
   uint16_t             op_in_trx = 0;
   uint64_t             virtual_op = 0;
   fc::time_point_sec   timestamp;
   api_operation        op;
   string               fee_payer;
};


struct api_account_object
{
   api_account_object( const database_api::api_account_object& a ) :
         id( a.id ),
         name( a.name ),
         owner( a.owner ),
         active( a.active ),
         memo_key( a.memo_key ),
         json_metadata( a.json_metadata ),
         voting_proxy( a.voting_proxy ),
         balance( a.balance ),
         sponsoring_account(a.sponsoring_account),
         vesting_shares( a.vesting_shares ),
         vesting_withdraw_rate( a.vesting_withdraw_rate ),
         to_withdraw( a.to_withdraw )
   {
      sponsored_accounts.insert( sponsored_accounts.end(), a.sponsored_accounts.begin(), a.sponsored_accounts.end() );
      witness_votes.insert( witness_votes.end(), a.witness_votes.begin(), a.witness_votes.end() );

   }


   api_account_object(){}

   chain::account_id_type  id;

   account_name_type       name;
   authority               owner;
   authority               active;
   public_key_type         memo_key;
   string                  json_metadata;
   account_name_type       voting_proxy;

   asset                   balance;

   std::vector<account_name_type>       sponsored_accounts;
   account_name_type                    sponsoring_account;
   vector<account_name_type>            witness_votes;

   asset         vesting_shares;
   asset         vesting_withdraw_rate;

   share_type        to_withdraw;

};

struct extended_account : public api_account_object
{
   extended_account(){}
   extended_account( const database_api::api_account_object& a ) :
         api_account_object( a ) {}


   asset                                   vesting_balance;  /// convert vesting_shares to vesting sophiatx
   map< uint64_t, api_operation_object >   transfer_history; /// transfer to/from vesting
   map< uint64_t, api_operation_object >   other_history;
   set< string >                           witness_votes;
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

         current_supply( o.current_supply ),
         total_vesting_shares( o.total_vesting_shares ),
         maximum_block_size( o.maximum_block_size ),
         current_aslot( o.current_aslot ),
         recent_slots_filled( o.recent_slots_filled ),
         participation_count( o.participation_count ),
         witness_required_vesting( o.witness_required_vesting ),
         last_irreversible_block_num( o.last_irreversible_block_num )

   {}

   uint32_t          head_block_number = 0;
   block_id_type     head_block_id;
   time_point_sec    time;
   account_name_type current_witness;

   asset             current_supply;
   asset             total_vesting_shares;

   uint32_t          maximum_block_size = 0;
   uint64_t          current_aslot = 0;
   fc::uint128_t     recent_slots_filled;
   uint8_t           participation_count = 0;
   asset             witness_required_vesting;


   uint32_t          last_irreversible_block_num = 0;

   int32_t           average_block_size = 0;
};


struct api_chain_properties
{
   api_chain_properties() {}
   api_chain_properties( const chain::chain_properties& c ) :
         account_creation_fee( c.account_creation_fee ),
         maximum_block_size( c.maximum_block_size )
   {}

   asset          account_creation_fee;
   uint32_t       maximum_block_size = SOPHIATX_MIN_BLOCK_SIZE_LIMIT * 2;
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
         votes( w.votes ),
         virtual_last_update( w.virtual_last_update ),
         virtual_position( w.virtual_position ),
         virtual_scheduled_time( w.virtual_scheduled_time ),
         running_version( w.running_version ),
         hardfork_version_vote( w.hardfork_version_vote ),
         hardfork_time_vote( w.hardfork_time_vote )
   {
      for(const auto&i : w.submitted_exchange_rates)
         submitted_exchange_rates.insert(i);
   }

   chain::witness_id_type  id;
   account_name_type       owner;
   time_point_sec          created;
   string                  url;
   uint32_t                total_missed = 0;
   uint64_t                last_aslot = 0;
   uint64_t                last_confirmed_block_num = 0;
   public_key_type         signing_key;
   api_chain_properties    props;
   flat_map<asset_symbol_type, chain::submitted_exchange_rate> submitted_exchange_rates;

   share_type              votes;
   fc::uint128_t           virtual_last_update;
   fc::uint128_t           virtual_position;
   fc::uint128_t           virtual_scheduled_time = fc::uint128_t::max_value();
   version                 running_version;
   hardfork_version        hardfork_version_vote;
   time_point_sec          hardfork_time_vote = SOPHIATX_GENESIS_TIME;
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

   chain::witness_schedule_id_type      id;
   fc::uint128_t                 current_virtual_time;
   uint32_t                      next_shuffle_block_num = 1;
   vector< account_name_type >   current_shuffled_witnesses;
   uint8_t                       num_scheduled_witnesses = 1;
   uint8_t                       top19_weight = 1;
   uint8_t                       timeshare_weight = 5;
   uint32_t                      witness_pay_normalization_factor = 25;
   api_chain_properties          median_props;
   version                       majority_version;
   uint8_t                       max_voted_witnesses           = SOPHIATX_MAX_VOTED_WITNESSES_HF0;
   uint8_t                       max_runner_witnesses          = SOPHIATX_MAX_RUNNER_WITNESSES_HF0;
   uint8_t                       hardfork_required_witnesses   = SOPHIATX_HARDFORK_REQUIRED_WITNESSES;
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
         sophiatx_balance( e.sophiatx_balance ),
         pending_fee( e.pending_fee ),
         to_approved( e.to_approved ),
         disputed( e.disputed ),
         agent_approved( e.agent_approved )
   {}

   chain::escrow_id_type    id;
   uint32_t          escrow_id = 20;
   account_name_type from;
   account_name_type to;
   account_name_type agent;
   time_point_sec    ratification_deadline;
   time_point_sec    escrow_expiration;
   asset             sophiatx_balance;
   asset             pending_fee;
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

   chain::application_id_type                      id;
   string                                          name;
   account_name_type                               author;
   string                                          url;
   string                                          metadata;
   application_price_param                         price_param;
};

struct scheduled_hardfork
{
   hardfork_version     hf_version;
   fc::time_point_sec   live_time;
};


struct get_version_info
{
   get_version_info() {}
   get_version_info( fc::string bc_v, fc::string s_v, fc::string fc_v, fc::string ci_v )
         :blockchain_version( bc_v ), sophiatx_revision( s_v ), fc_revision( fc_v ), chain_id(ci_v) {}

   fc::string blockchain_version;
   fc::string sophiatx_revision;
   fc::string fc_revision;
   fc::string chain_id;
};

struct api_hardfork_property_object
{
   api_hardfork_property_object( const database_api::api_hardfork_property_object& h ) :
         id( h.id ),
         last_hardfork( h.last_hardfork ),
         current_hardfork_version( h.current_hardfork_version ),
         next_hardfork( h.next_hardfork ),
         next_hardfork_time( h.next_hardfork_time )
   {
      size_t n = h.processed_hardforks.size();
      processed_hardforks.reserve( n );

      for( size_t i = 0; i < n; i++ )
         processed_hardforks.push_back( h.processed_hardforks[i] );
   }

   api_hardfork_property_object() {}

   chain::hardfork_property_id_type     id;
   vector< fc::time_point_sec >         processed_hardforks;
   uint32_t                             last_hardfork;
   protocol::hardfork_version           current_hardfork_version;
   protocol::hardfork_version           next_hardfork;
   fc::time_point_sec                   next_hardfork_time;
};

} } } // sophiatx::plugins::database_api

FC_REFLECT( sophiatx::plugins::alexandria_api::key_pair_st, (pub_key)(wif_priv_key) )

FC_REFLECT_ENUM( sophiatx::plugins::alexandria_api::authority_type, (owner)(active) )

FC_REFLECT( sophiatx::plugins::alexandria_api::memo_data, (nonce)(check)(encrypted) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_signed_transaction,
            (ref_block_num)(ref_block_prefix)(expiration)(operations)(extensions)(signatures)(transaction_id)(block_num)(transaction_num) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_signed_block,
            (previous)(timestamp)(witness)(transaction_merkle_root)(extensions)(witness_signature)(transactions)(block_id)(signing_key)(transaction_ids) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_operation_object,
            (trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(op)(fee_payer) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_feed_history_object,
            (id)
            (current_median_price)
            (price_history) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_account_object,
            (id)(name)(owner)(active)(memo_key)(json_metadata)(voting_proxy)
                  (balance)
                  (vesting_shares)(vesting_withdraw_rate)(to_withdraw)
                  (witness_votes)(sponsored_accounts)(sponsoring_account) )

FC_REFLECT_DERIVED( sophiatx::plugins::alexandria_api::extended_account, (sophiatx::plugins::alexandria_api::api_account_object),
                    (vesting_balance)(transfer_history)(other_history)(witness_votes) )

FC_REFLECT( sophiatx::plugins::alexandria_api::extended_dynamic_global_properties,
            (head_block_number)(head_block_id)(time)
                  (current_witness)
                  (current_supply)
                  (total_vesting_shares)(witness_required_vesting)
                  (maximum_block_size)(current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)
                  (average_block_size) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_chain_properties,
            (account_creation_fee)(maximum_block_size) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_witness_object,
            (id)
                  (owner)
                  (created)
                  (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
                  (last_aslot)(last_confirmed_block_num)(signing_key)
                  (props)
                  (submitted_exchange_rates)
                  (running_version)
                  (hardfork_version_vote)(hardfork_time_vote) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_escrow_object,
            (id)(escrow_id)(from)(to)(agent)
                  (ratification_deadline)(escrow_expiration)
                  (sophiatx_balance)(pending_fee)
                  (to_approved)(agent_approved)(disputed) )


FC_REFLECT( sophiatx::plugins::alexandria_api::scheduled_hardfork,
            (hf_version)(live_time) )

FC_REFLECT( sophiatx::plugins::alexandria_api::get_version_info,
            (blockchain_version)(sophiatx_revision)(fc_revision)(chain_id) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_application_object,
            (id)
                  (name)
                  (author)
                  (url)
                  (metadata)
                  (price_param) )

FC_REFLECT( sophiatx::plugins::alexandria_api::api_witness_schedule_object,
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

FC_REFLECT( sophiatx::plugins::alexandria_api::api_hardfork_property_object,
            (id)
                  (processed_hardforks)
                  (last_hardfork)
                  (current_hardfork_version)
                  (next_hardfork)
                  (next_hardfork_time)
)

