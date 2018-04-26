#pragma once
#include <steem/chain/account_object.hpp>
#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/transaction_object.hpp>
#include <steem/chain/witness_objects.hpp>
#include <steem/chain/database.hpp>

namespace steem { namespace plugins { namespace database_api {

using namespace steem::chain;

typedef change_recovery_account_request_object api_change_recovery_account_request_object;
typedef block_summary_object                   api_block_summary_object;
typedef dynamic_global_property_object         api_dynamic_global_property_object;
typedef escrow_object                          api_escrow_object;
typedef witness_vote_object                    api_witness_vote_object;
typedef reward_fund_object                     api_reward_fund_object;

struct api_account_object
{
   api_account_object( const account_object& a, const database& db ) :
      id( a.id ),
      name( a.name ),
      memo_key( a.memo_key ),
      json_metadata( to_string( a.json_metadata ) ),
      proxy( a.proxy ),
      last_account_update( a.last_account_update ),
      created( a.created ),
      recovery_account( a.recovery_account ),
      reset_account( a.reset_account ),
      last_account_recovery( a.last_account_recovery ),
      balance( a.balance ),
      vesting_shares( a.vesting_shares ),
      vesting_withdraw_rate( a.vesting_withdraw_rate ),
      next_vesting_withdrawal( a.next_vesting_withdrawal ),
      withdrawn( a.withdrawn ),
      to_withdraw( a.to_withdraw ),
      witnesses_voted_for( a.witnesses_voted_for )
   {
      size_t n = a.proxied_vsf_votes.size();
      proxied_vsf_votes.reserve( n );
      for( size_t i=0; i<n; i++ )
         proxied_vsf_votes.push_back( a.proxied_vsf_votes[i] );

      const auto& auth = db.get< account_authority_object, by_account >( name );
      owner = authority( auth.owner );
      active = authority( auth.active );
      last_owner_update = auth.last_owner_update;
#ifdef STEEM_ENABLE_SMT
      const auto& by_control_account_index = db.get_index<smt_token_index>().indices().get<by_control_account>();
      auto smt_obj_itr = by_control_account_index.find( name );
      is_smt = smt_obj_itr != by_control_account_index.end();
#endif
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

   asset             balance;

   asset             vesting_shares;
   asset             vesting_withdraw_rate;
   time_point_sec    next_vesting_withdrawal;
   share_type        withdrawn;
   share_type        to_withdraw;

   vector< share_type > proxied_vsf_votes;

   uint16_t          witnesses_voted_for;

};

struct api_owner_authority_history_object
{
   api_owner_authority_history_object( const owner_authority_history_object& o ) :
      id( o.id ),
      account( o.account ),
      previous_owner_authority( authority( o.previous_owner_authority ) ),
      last_valid_time( o.last_valid_time )
   {}

   api_owner_authority_history_object() {}

   owner_authority_history_id_type  id;

   account_name_type                account;
   authority                        previous_owner_authority;
   time_point_sec                   last_valid_time;
};

struct api_account_recovery_request_object
{
   api_account_recovery_request_object( const account_recovery_request_object& o ) :
      id( o.id ),
      account_to_recover( o.account_to_recover ),
      new_owner_authority( authority( o.new_owner_authority ) ),
      expires( o.expires )
   {}

   api_account_recovery_request_object() {}

   account_recovery_request_id_type id;
   account_name_type                account_to_recover;
   authority                        new_owner_authority;
   time_point_sec                   expires;
};

struct api_account_history_object
{

};

struct api_feed_history_object
{
   api_feed_history_object( const feed_history_object& f ) :
      id( f.id ),
      current_median_history( f.current_median_history ),
      price_history( f.price_history.begin(), f.price_history.end() )
   {}

   api_feed_history_object() {}

   feed_history_id_type id;
   price                current_median_history;
   deque< price >       price_history;
};

struct api_witness_object
{
   api_witness_object( const witness_object& w ) :
      id( w.id ),
      owner( w.owner ),
      created( w.created ),
      url( to_string( w.url ) ),
      total_missed( w.total_missed ),
      last_aslot( w.last_aslot ),
      last_confirmed_block_num( w.last_confirmed_block_num ),
      signing_key( w.signing_key ),
      props( w.props ),
      sbd_exchange_rate( w.sbd_exchange_rate ),
      last_sbd_exchange_update( w.last_sbd_exchange_update ),
      votes( w.votes ),
      virtual_last_update( w.virtual_last_update ),
      virtual_position( w.virtual_position ),
      virtual_scheduled_time( w.virtual_scheduled_time ),
      last_work( w.last_work ),
      running_version( w.running_version ),
      hardfork_version_vote( w.hardfork_version_vote ),
      hardfork_time_vote( w.hardfork_time_vote )
   {}

   api_witness_object() {}

   witness_id_type   id;
   account_name_type owner;
   time_point_sec    created;
   string            url;
   uint32_t          total_missed = 0;
   uint64_t          last_aslot = 0;
   uint64_t          last_confirmed_block_num = 0;
   public_key_type   signing_key;
   chain_properties  props;
   price             sbd_exchange_rate;
   time_point_sec    last_sbd_exchange_update;
   share_type        votes;
   fc::uint128       virtual_last_update;
   fc::uint128       virtual_position;
   fc::uint128       virtual_scheduled_time;
   digest_type       last_work;
   version           running_version;
   hardfork_version  hardfork_version_vote;
   time_point_sec    hardfork_time_vote;
};

struct api_witness_schedule_object
{
   api_witness_schedule_object() {}

   api_witness_schedule_object( const witness_schedule_object& wso) :
      id( wso.id ),
      current_virtual_time( wso.current_virtual_time ),
      next_shuffle_block_num( wso.next_shuffle_block_num ),
      num_scheduled_witnesses( wso.num_scheduled_witnesses ),
      top19_weight( wso.top19_weight ),
      timeshare_weight( wso.timeshare_weight ),
      witness_pay_normalization_factor( wso.witness_pay_normalization_factor ),
      median_props( wso.median_props ),
      majority_version( wso.majority_version ),
      max_voted_witnesses( wso.max_voted_witnesses ),
      max_runner_witnesses( wso.max_runner_witnesses ),
      hardfork_required_witnesses( wso.hardfork_required_witnesses )
   {
      size_t n = wso.current_shuffled_witnesses.size();
      current_shuffled_witnesses.reserve( n );
      std::transform(wso.current_shuffled_witnesses.begin(), wso.current_shuffled_witnesses.end(),
                     std::back_inserter(current_shuffled_witnesses),
                     [](const account_name_type& s) -> std::string { return s; } );
                     // ^ fixed_string std::string operator used here.
   }

   witness_schedule_id_type   id;

   fc::uint128                current_virtual_time;
   uint32_t                   next_shuffle_block_num;
   vector<string>             current_shuffled_witnesses;   // fc::array<account_name_type,...> -> vector<string>
   uint8_t                    num_scheduled_witnesses;
   uint8_t                    top19_weight;
   uint8_t                    timeshare_weight;
   uint32_t                   witness_pay_normalization_factor;
   chain_properties           median_props;
   version                    majority_version;

   uint8_t                    max_voted_witnesses;
   uint8_t                    max_runner_witnesses;
   uint8_t                    hardfork_required_witnesses;
};

struct api_signed_block_object : public signed_block
{
   api_signed_block_object( const signed_block& block ) : signed_block( block )
   {
      block_id = id();
      signing_key = signee();
      transaction_ids.reserve( transactions.size() );
      for( const signed_transaction& tx : transactions )
         transaction_ids.push_back( tx.id() );
   }
   api_signed_block_object() {}

   block_id_type                 block_id;
   public_key_type               signing_key;
   vector< transaction_id_type > transaction_ids;
};

struct api_hardfork_property_object
{
   api_hardfork_property_object( const hardfork_property_object& h ) :
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

   hardfork_property_id_type     id;
   vector< fc::time_point_sec >  processed_hardforks;
   uint32_t                      last_hardfork;
   protocol::hardfork_version    current_hardfork_version;
   protocol::hardfork_version    next_hardfork;
   fc::time_point_sec            next_hardfork_time;
};




} } } // steem::plugins::database_api


FC_REFLECT( steem::plugins::database_api::api_account_object,
             (id)(name)(owner)(active)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)
             (recovery_account)(last_account_recovery)(reset_account)
             (balance)
             (vesting_shares)(vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)
             (proxied_vsf_votes)(witnesses_voted_for)
          )

FC_REFLECT( steem::plugins::database_api::api_owner_authority_history_object,
             (id)
             (account)
             (previous_owner_authority)
             (last_valid_time)
          )

FC_REFLECT( steem::plugins::database_api::api_account_recovery_request_object,
             (id)
             (account_to_recover)
             (new_owner_authority)
             (expires)
          )

FC_REFLECT( steem::plugins::database_api::api_feed_history_object,
             (id)
             (current_median_history)
             (price_history)
          )

FC_REFLECT( steem::plugins::database_api::api_witness_object,
             (id)
             (owner)
             (created)
             (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_aslot)(last_confirmed_block_num)(signing_key)
             (props)
             (sbd_exchange_rate)(last_sbd_exchange_update)
             (last_work)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )

FC_REFLECT( steem::plugins::database_api::api_witness_schedule_object,
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

FC_REFLECT_DERIVED( steem::plugins::database_api::api_signed_block_object, (steem::protocol::signed_block),
                     (block_id)
                     (signing_key)
                     (transaction_ids)
                  )

FC_REFLECT( steem::plugins::database_api::api_hardfork_property_object,
            (id)
            (processed_hardforks)
            (last_hardfork)
            (current_hardfork_version)
            (next_hardfork)
            (next_hardfork_time)
          )
