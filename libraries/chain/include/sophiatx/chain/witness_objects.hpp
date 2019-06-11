#pragma once

#include <sophiatx/protocol/authority.hpp>
#include <sophiatx/protocol/sophiatx_operations.hpp>
#include <sophiatx/protocol/get_config.hpp>

#include <sophiatx/chain/sophiatx_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>


namespace sophiatx { namespace chain {
namespace bip = boost::interprocess;

   using sophiatx::protocol::digest_type;
   using sophiatx::protocol::public_key_type;
   using sophiatx::protocol::version;
   using sophiatx::protocol::hardfork_version;
   using sophiatx::protocol::price;
   using sophiatx::protocol::asset;
   using sophiatx::protocol::asset_symbol_type;
   using sophiatx::protocol::chain_properties;

   struct submitted_exchange_rate{
      price            rate;
      time_point_sec   last_change;
   };


struct shared_chain_properties
{
   /**
    *  This fee, paid in SOPHIATX, is converted into VESTING SHARES for the new account. Accounts
    *  without vesting shares cannot earn usage rations and therefore are powerless. This minimum
    *  fee requires all accounts to have some kind of commitment to the network that includes the
    *  ability to vote and make transactions.
    */
   asset account_creation_fee = asset( SOPHIATX_MIN_ACCOUNT_CREATION_FEE, SOPHIATX_SYMBOL );

   /**
    *  This witnesses vote for the maximum_block_size which is used by the network
    *  to tune rate limiting and capacity
    */
   uint32_t          maximum_block_size = SOPHIATX_MIN_BLOCK_SIZE_LIMIT * 2;

   typedef bip::allocator< shared_chain_properties, bip::managed_mapped_file::segment_manager >                  allocator_type;

   typedef bip::allocator< std::pair< asset_symbol_type, price >, bip::managed_mapped_file::segment_manager >     price_feed_allocator_type;

   typedef bip::flat_map< asset_symbol_type, price, std::less< asset_symbol_type >, price_feed_allocator_type > price_feeds_map;


   price_feeds_map price_feeds;

   template< typename Allocator >
   shared_chain_properties( const Allocator& alloc ) :
         price_feeds( price_feed_allocator_type( alloc.get_segment_manager() ) ) {}

   shared_chain_properties& operator=( const chain_properties& a ){
      price_feeds.clear();
      maximum_block_size = a.maximum_block_size;
      account_creation_fee = a.account_creation_fee;
      for( const auto& item : a.price_feeds )
         price_feeds.insert( item );
      return *this;
   };

   operator chain_properties()const{
      chain_properties result;
      result.maximum_block_size=maximum_block_size;
      result.account_creation_fee = account_creation_fee;
      for( const auto& item: price_feeds)
         result.price_feeds.insert(item);
      return result;

   }
};

/**
    *  All witnesses with at least 1% net positive approval and
    *  at least 2 weeks old are able to participate in block
    *  production.
    */
   class witness_object : public object< witness_object_type, witness_object >
   {
      witness_object() = delete;

      public:
         enum witness_schedule_type
         {
            top19,
            timeshare,
            none
         };

         template< typename Constructor, typename Allocator >
         witness_object( Constructor&& c, allocator< Allocator > a )
            :url( a ), props( a ), submitted_exchange_rates( submitted_exchange_rates_allocator_type( a.get_segment_manager() ) )
         {
            c( *this );
         }

         id_type           id;

         /** the account that has authority over this witness */
         account_name_type owner;
         time_point_sec    created;
         shared_string     url;
         uint32_t          total_missed = 0;
         uint64_t          last_aslot = 0;
         uint64_t          last_confirmed_block_num = 0;

         /**
          *  This is the key used to sign blocks on behalf of this witness
          */
         public_key_type   signing_key;

         shared_chain_properties  props;

         typedef bip::allocator< witness_object, bip::managed_mapped_file::segment_manager >                                allocator_type;

         typedef bip::allocator< std::pair< asset_symbol_type, submitted_exchange_rate >, bip::managed_mapped_file::segment_manager >     submitted_exchange_rates_allocator_type;

         typedef bip::flat_map< asset_symbol_type, submitted_exchange_rate, std::less< asset_symbol_type >, submitted_exchange_rates_allocator_type > submitted_exchange_rates_map;

         submitted_exchange_rates_map submitted_exchange_rates;


         /**
          *  The total votes for this witness. This determines how the witness is ranked for
          *  scheduling.  The top N witnesses by votes are scheduled every round, every one
          *  else takes turns being scheduled proportional to their votes.
          */
         share_type        votes;
         witness_schedule_type schedule = none; /// How the witness was scheduled the last time it was scheduled

         /**
          * These fields are used for the witness scheduling algorithm which uses
          * virtual time to ensure that all witnesses are given proportional time
          * for producing blocks.
          *
          * @ref votes is used to determine speed. The @ref virtual_scheduled_time is
          * the expected time at which this witness should complete a virtual lap which
          * is defined as the position equal to 1000 times MAXVOTES.
          *
          * virtual_scheduled_time = virtual_last_update + (1000*MAXVOTES - virtual_position) / votes
          *
          * Every time the number of votes changes the virtual_position and virtual_scheduled_time must
          * update.  There is a global current virtual_scheduled_time which gets updated every time
          * a witness is scheduled.  To update the virtual_position the following math is performed.
          *
          * virtual_position       = virtual_position + votes * (virtual_current_time - virtual_last_update)
          * virtual_last_update    = virtual_current_time
          * votes                  += delta_vote
          * virtual_scheduled_time = virtual_last_update + (1000*MAXVOTES - virtual_position) / votes
          *
          * @defgroup virtual_time Virtual Time Scheduling
          */
         ///@{
         fc::uint128       virtual_last_update;
         fc::uint128       virtual_position;
         fc::uint128       virtual_scheduled_time = fc::uint128::max_value();
         ///@}

         bool              stopped = false;

         /**
          * This field represents the SophiaTX blockchain version the witness is running.
          */
         version           running_version;

         hardfork_version  hardfork_version_vote;
         time_point_sec    hardfork_time_vote = SOPHIATX_GENESIS_TIME;
   };


   class witness_vote_object : public object< witness_vote_object_type, witness_vote_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         witness_vote_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         witness_vote_object(){}

         id_type           id;

         account_name_type witness;
         account_name_type account;
   };

   class witness_schedule_object : public object< witness_schedule_object_type, witness_schedule_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         witness_schedule_object( Constructor&& c, allocator< Allocator > a ) :
         current_shuffled_witnesses(protocol::sophiatx_config::get<uint32_t>("SOPHIATX_MAX_WITNESSES")),
         max_voted_witnesses(protocol::sophiatx_config::get<uint8_t>("SOPHIATX_MAX_VOTED_WITNESSES_HF0")),
         max_runner_witnesses(protocol::sophiatx_config::get<uint8_t>("SOPHIATX_MAX_RUNNER_WITNESSES_HF0")),
         hardfork_required_witnesses(protocol::sophiatx_config::get<uint8_t>("SOPHIATX_HARDFORK_REQUIRED_WITNESSES"))
         {
            c( *this );
         }

         witness_schedule_object(){}

         id_type                                                           id;

         fc::uint128                                                       current_virtual_time;
         uint32_t                                                          next_shuffle_block_num = 1;
         std::vector< account_name_type >                                  current_shuffled_witnesses;
         uint8_t                                                           num_scheduled_witnesses = 1;
         uint8_t                                                           top19_weight = 21;
         uint8_t                                                           timeshare_weight = 63;
         uint32_t                                                          witness_pay_normalization_factor = 25;
         chain_properties                                                  median_props;
         version                                                           majority_version;

         uint8_t max_voted_witnesses;
         uint8_t max_runner_witnesses;
         uint8_t hardfork_required_witnesses;
   };



   struct by_vote_name;
   struct by_name;
   struct by_stopped;
   struct by_schedule_time;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      witness_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< witness_object, witness_id_type, &witness_object::id > >,
         ordered_non_unique< tag< by_stopped >, member< witness_object, bool, &witness_object::stopped > >,
         ordered_unique< tag< by_name >, member< witness_object, account_name_type, &witness_object::owner > >,
         ordered_unique< tag< by_vote_name >,
            composite_key< witness_object,
               member< witness_object, share_type, &witness_object::votes >,
               member< witness_object, account_name_type, &witness_object::owner >
            >,
            composite_key_compare< std::greater< share_type >, std::less< account_name_type > >
         >,
         ordered_unique< tag< by_schedule_time >,
            composite_key< witness_object,
               member< witness_object, fc::uint128, &witness_object::virtual_scheduled_time >,
               member< witness_object, witness_id_type, &witness_object::id >
            >
         >
      >,
      allocator< witness_object >
   > witness_index;

   struct by_account_witness;
   struct by_witness_account;
   typedef multi_index_container<
      witness_vote_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< witness_vote_object, witness_vote_id_type, &witness_vote_object::id > >,
         ordered_unique< tag< by_account_witness >,
            composite_key< witness_vote_object,
               member< witness_vote_object, account_name_type, &witness_vote_object::account >,
               member< witness_vote_object, account_name_type, &witness_vote_object::witness >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
         >,
         ordered_unique< tag< by_witness_account >,
            composite_key< witness_vote_object,
               member< witness_vote_object, account_name_type, &witness_vote_object::witness >,
               member< witness_vote_object, account_name_type, &witness_vote_object::account >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
         >
      >, // indexed_by
      allocator< witness_vote_object >
   > witness_vote_index;

   typedef multi_index_container<
      witness_schedule_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< witness_schedule_object, witness_schedule_id_type, &witness_schedule_object::id > >
      >,
      allocator< witness_schedule_object >
   > witness_schedule_index;

} }

FC_REFLECT_ENUM( sophiatx::chain::witness_object::witness_schedule_type, (top19)(timeshare)(none) )

FC_REFLECT( sophiatx::chain::submitted_exchange_rate, (rate)(last_change))

FC_REFLECT( sophiatx::chain::witness_object,
             (id)
             (owner)
             (created)
             (url)(votes)(schedule)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_aslot)(last_confirmed_block_num)(signing_key)
             (props)
             (submitted_exchange_rates)
             (stopped)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::witness_object, sophiatx::chain::witness_index )

FC_REFLECT( sophiatx::chain::witness_vote_object, (id)(witness)(account) )
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::witness_vote_object, sophiatx::chain::witness_vote_index )

FC_REFLECT( sophiatx::chain::witness_schedule_object,
             (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_witnesses)(num_scheduled_witnesses)
             (top19_weight)(timeshare_weight)(witness_pay_normalization_factor)
             (median_props)(majority_version)
             (max_voted_witnesses)
             (max_runner_witnesses)
             (hardfork_required_witnesses)
          )
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::witness_schedule_object, sophiatx::chain::witness_schedule_index )

FC_REFLECT( sophiatx::chain::shared_chain_properties,
            (account_creation_fee)
                  (maximum_block_size)
                  (price_feeds)
)
