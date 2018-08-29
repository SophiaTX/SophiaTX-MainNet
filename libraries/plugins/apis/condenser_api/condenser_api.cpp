#include <sophiatx/plugins/condenser_api/condenser_api.hpp>
#include <sophiatx/plugins/condenser_api/condenser_api_plugin.hpp>

#include <sophiatx/plugins/database_api/database_api_plugin.hpp>
#include <sophiatx/plugins/block_api/block_api_plugin.hpp>
#include <sophiatx/plugins/account_history_api/account_history_api_plugin.hpp>
#include <sophiatx/plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <sophiatx/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <sophiatx/plugins/witness_api/witness_api_plugin.hpp>
#include <sophiatx/plugins/custom_api/custom_api_plugin.hpp>
#include <sophiatx/plugins/subscribe_api/subscribe_api_plugin.hpp>

#include <sophiatx/utilities/git_revision.hpp>

#include <sophiatx/chain/util/uint256.hpp>

#include <fc/git_revision.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#define CHECK_ARG_SIZE( s ) \
   FC_ASSERT( args.size() == s, "Expected #s argument(s), was ${n}", ("n", args.size()) );

namespace sophiatx { namespace plugins { namespace condenser_api {

namespace detail
{
   class condenser_api_impl
   {
      public:
         condenser_api_impl() : _db( appbase::app().get_plugin< chain::chain_plugin >().db() ) {}

         DECLARE_API_IMPL(
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
            (custom_object_subscription)
  //          (get_account_votes)
            (get_account_history)
            (broadcast_transaction)
            (broadcast_transaction_synchronous)
            (broadcast_block)
            (get_applications)
            (get_promotion_pool_balance)
            (list_received_documents)
            (get_application_buyings)
         )



         chain::database& _db;

         std::shared_ptr< database_api::database_api > _database_api;
         std::shared_ptr< block_api::block_api > _block_api;
         std::shared_ptr< account_history::account_history_api > _account_history_api;
         std::shared_ptr< account_by_key::account_by_key_api > _account_by_key_api;
         std::shared_ptr< network_broadcast_api::network_broadcast_api > _network_broadcast_api;
         std::shared_ptr< witness::witness_api > _witness_api;
         std::shared_ptr< custom::custom_api > _custom_api;
         std::shared_ptr< subscribe::subscribe_api> _subscribe_api;
   };

   DEFINE_API_IMPL( condenser_api_impl, get_version )
   {
      CHECK_ARG_SIZE( 0 )
      return get_version_return
      (
         fc::string( SOPHIATX_BLOCKCHAIN_VERSION ),
         fc::string( sophiatx::utilities::git_revision_sha ),
         fc::string( fc::git_revision_sha ),
         fc::string( _database_api->get_dynamic_global_properties({}).chain_id  )
      );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_state )
   {
      CHECK_ARG_SIZE( 1 )
      string path = args[0].as< string >();

      state _state;
      _state.props         = get_dynamic_global_properties( {} );
      _state.current_route = path;
      _state.feed_price    = _database_api->get_current_price_feed( {} );

      try
      {
         if( path.size() && path[0] == '/' )
            path = path.substr(1); /// remove '/' from front

         /// END FETCH CATEGORY STATE

         set<string> accounts;

         vector<string> part; part.reserve(4);
         boost::split( part, path, boost::is_any_of("/") );
         part.resize(std::max( part.size(), size_t(4) ) ); // at least 4

         auto tag = fc::to_lower( part[1] );

         if( part[0].size() && part[0][0] == '@' ) {
            auto acnt = part[0].substr(1);
            _state.accounts[acnt] = extended_account( database_api::api_account_object( _db.get_account( acnt ), _db ) );

            auto& eacnt = _state.accounts[acnt];
            if( part[1] == "transfers" )
            {
               if( _account_history_api )
               {
                  legacy_operation l_op;
                  legacy_operation_conversion_visitor visitor( l_op );
                  auto history = _account_history_api->get_account_history( { acnt, uint64_t(-1), 1000 } ).history;
                  for( auto& item : history )
                  {
                     switch( item.second.op.which() ) {
                        case operation::tag<transfer_to_vesting_operation>::value:
                        case operation::tag<withdraw_vesting_operation>::value:
                        case operation::tag<interest_operation>::value:
                        case operation::tag<transfer_operation>::value:
                        case operation::tag<escrow_transfer_operation>::value:
                        case operation::tag<escrow_approve_operation>::value:
                        case operation::tag<escrow_dispute_operation>::value:
                        case operation::tag<escrow_release_operation>::value:
                           if( item.second.op.visit( visitor ) )
                           {
                              eacnt.transfer_history.emplace( item.first, api_operation_object( item.second, visitor.l_op ) );
                           }
                           break;
                        case operation::tag<account_witness_vote_operation>::value:
                        case operation::tag<account_witness_proxy_operation>::value:
                           //TODO_SOPHIA Shall we return the vote history???
                        //   eacnt.vote_history[item.first] =  item.second;
                           break;
                        case operation::tag<account_create_operation>::value:
                        case operation::tag<account_update_operation>::value:
                        case operation::tag<witness_update_operation>::value:
                        case operation::tag<witness_stop_operation>::value:

                        case operation::tag<custom_operation>::value:
                        case operation::tag<producer_reward_operation>::value:
                        default:
                           if( item.second.op.visit( visitor ) )
                           {
                              eacnt.other_history.emplace( item.first, api_operation_object( item.second, visitor.l_op ) );
                           }
                     }
                  }
               }
            }
            //else if( part[1].size() == 0 || part[1] == "blog" )

         }
         /// pull a complete discussion
         else if( part[1].size() && part[1][0] == '@' )
         {
            auto account  = part[1].substr( 1 );
            auto slug     = part[2];

            string key = account + "/" + slug;

         }
         else if( part[0] == "witnesses" || part[0] == "~witnesses")
         {
            auto wits = get_witnesses_by_vote( { fc::variant(""), fc::variant(50) } );
            for( const auto& w : wits )
            {
               _state.witnesses[w.owner] = w;
            }
         }
         else if( part[0] == "payout"  )
         {
            //TODO_SOPHIA - payouts from mining
         }
         else if( part[0] == "votes"  )
         {
            //TODO_SOPHIA Shall we return the vote history???
         }
         else {
            elog( "What... no matches" );
         }

         for( const auto& a : accounts )
         {
            _state.accounts.erase("");
            _state.accounts[a] = extended_account( database_api::api_account_object( _db.get_account( a ), _db ) );

         }


         _state.witness_schedule = _database_api->get_witness_schedule( {} );

      }
      catch ( const fc::exception& e )
      {
         _state.error = e.to_detail_string();
      }

      return _state;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_active_witnesses )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_active_witnesses( {} ).witnesses;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_block_header )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _block_api, "block_api_plugin not enabled." );
      return _block_api->get_block_header( { args[0].as< uint32_t >() } ).header;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_block )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _block_api, "block_api_plugin not enabled." );
      get_block_return result;
      auto b = _block_api->get_block( { args[0].as< uint32_t >() } ).block;

      if( b )
         result = legacy_signed_block( *b );

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_ops_in_block )
   {
      FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
      FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );

      auto ops = _account_history_api->get_ops_in_block( { args[0].as< uint32_t >(), args[1].as< bool >() } ).ops;
      get_ops_in_block_return result;

      legacy_operation l_op;
      legacy_operation_conversion_visitor visitor( l_op );

      for( auto& op_obj : ops )
      {
         if( op_obj.op.visit( visitor) )
         {
            result.push_back( api_operation_object( op_obj, visitor.l_op ) );
         }
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_config )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_config( {} );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_dynamic_global_properties )
   {
      CHECK_ARG_SIZE( 0 )
      get_dynamic_global_properties_return gpo = _database_api->get_dynamic_global_properties( {} );
      if( _witness_api )
      {
         auto reserve_ratio = _witness_api->get_reserve_ratio( {} );
         gpo.average_block_size = reserve_ratio.average_block_size;
      }

      return gpo;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_chain_properties )
   {
      CHECK_ARG_SIZE( 0 )
      return legacy_chain_properties( _database_api->get_witness_schedule( {} ).median_props );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_current_median_history_price )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->get_current_price_feed( {args[0].as< asset_symbol_type >()} );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_feed_history )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->get_feed_history( {args[0].as< asset_symbol_type>()} );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_witness_schedule )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_witness_schedule( {} );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_hardfork_version )
   {
      CHECK_ARG_SIZE( 0 )
      return _database_api->get_hardfork_properties( {} ).current_hardfork_version;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_next_scheduled_hardfork )
   {
      CHECK_ARG_SIZE( 0 )
      scheduled_hardfork shf;
      const auto& hpo = _db.get( hardfork_property_id_type() );
      shf.hf_version = hpo.next_hardfork;
      shf.live_time = hpo.next_hardfork_time;
      return shf;
   }


   DEFINE_API_IMPL( condenser_api_impl, get_key_references )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _account_by_key_api, "account_history_api_plugin not enabled." );

      return _account_by_key_api->get_key_references( { args[0].as< vector< public_key_type > >() } ).accounts;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_accounts )
   {
      CHECK_ARG_SIZE(1)
      vector< account_name_type > names = args[0].as< vector< account_name_type > >();

      const auto& idx  = _db.get_index< account_index >().indices().get< by_name >();
      const auto& vidx = _db.get_index< witness_vote_index >().indices().get< by_account_witness >();
      vector< extended_account > results;
      results.reserve(names.size());

      for( const auto& name: names )
      {
         auto itr = idx.find( name );
         if ( itr != idx.end() )
         {
            results.emplace_back( extended_account( database_api::api_account_object( *itr, _db ) ) );

            auto vitr = vidx.lower_bound( boost::make_tuple( itr->name, account_name_type() ) );
            while( vitr != vidx.end() && vitr->account == itr->name ) {
               results.back().witness_votes.insert( _db.get< witness_object, by_name >( vitr->witness ).owner );
               ++vitr;
            }
         }
      }

      return results;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_account_references )
   {
      FC_ASSERT( false, "condenser_api::get_account_references --- Needs to be refactored for SophiaTX." );
   }

   DEFINE_API_IMPL( condenser_api_impl, lookup_account_names )
   {
      CHECK_ARG_SIZE( 1 )
      vector< account_name_type > account_names = args[0].as< vector< account_name_type > >();

      vector< optional< api_account_object > > result;
      result.reserve( account_names.size() );

      for( auto& name : account_names )
      {
         auto itr = _db.find< account_object, by_name >( name );

         if( itr )
         {
            result.push_back( api_account_object( database_api::api_account_object( *itr, _db ) ) );
         }
         else
         {
            result.push_back( optional< api_account_object >() );
         }
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, lookup_accounts )
   {
      CHECK_ARG_SIZE( 2 )
      account_name_type lower_bound_name = args[0].as< account_name_type >();
      uint32_t limit = args[1].as< uint32_t >();

      FC_ASSERT( limit <= 1000 );
      const auto& accounts_by_name = _db.get_index< account_index, by_name >();
      set<string> result;

      for( auto itr = accounts_by_name.lower_bound( lower_bound_name );
           limit-- && itr != accounts_by_name.end();
           ++itr )
      {
         result.insert( itr->name );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_account_count )
   {
      CHECK_ARG_SIZE( 0 )
      return _db.get_index<account_index>().indices().size();
   }

   DEFINE_API_IMPL( condenser_api_impl, get_owner_history )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->find_owner_histories( { args[0].as< string >() } ).owner_auths;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_recovery_request )
   {
      CHECK_ARG_SIZE( 1 )
      get_recovery_request_return result;

      auto requests = _database_api->find_account_recovery_requests( { { args[0].as< account_name_type >() } } ).requests;

      if( requests.size() )
         result = requests[0];

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_escrow )
   {
      CHECK_ARG_SIZE( 2 )
      get_escrow_return result;

      auto escrows = _database_api->list_escrows( { { args }, 1, database_api::by_from_id } ).escrows;

      if( escrows.size()
         && escrows[0].from == args[0].as< account_name_type >()
         && escrows[0].escrow_id == args[1].as< uint32_t >() )
      {
         result = escrows[0];
      }

      return result;
   }



   DEFINE_API_IMPL( condenser_api_impl, get_witnesses )
   {
      CHECK_ARG_SIZE( 1 )
      vector< witness_id_type > witness_ids = args[0].as< vector< witness_id_type > >();

      get_witnesses_return result;
      result.reserve( witness_ids.size() );

      std::transform(
         witness_ids.begin(),
         witness_ids.end(),
         std::back_inserter(result),
         [this](witness_id_type id) -> optional< api_witness_object >
         {
            if( auto o = _db.find(id) )
               return api_witness_object( database_api::api_witness_object ( *o ) );
            return {};
         });

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_witness_by_account )
   {
      CHECK_ARG_SIZE( 1 )
      auto witnesses = _database_api->find_witnesses(
         {
            { args[0].as< account_name_type >() }
         }).witnesses;

      get_witness_by_account_return result;

      if( witnesses.size() )
         result = api_witness_object( witnesses[0] );

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_witnesses_by_vote )
   {
      CHECK_ARG_SIZE( 2 )
      account_name_type start_name = args[0].as< account_name_type >();
      vector< fc::variant > start_key;

      if( start_name == account_name_type() )
      {
         start_key.push_back( fc::variant( std::numeric_limits< int64_t >::max() ) );
         start_key.push_back( fc::variant( account_name_type() ) );
      }
      else
      {
         auto start = _database_api->list_witnesses( { args[0], 1, database_api::by_name } );

         if( start.witnesses.size() == 0 )
            return get_witnesses_by_vote_return();

         start_key.push_back( fc::variant( start.witnesses[0].votes ) );
         start_key.push_back( fc::variant( start.witnesses[0].owner ) );
      }

      auto limit = args[1].as< uint32_t >();
      auto witnesses = _database_api->list_witnesses( { fc::variant( start_key ), limit, database_api::by_vote_name } ).witnesses;

      get_witnesses_by_vote_return result;

      for( auto& w : witnesses )
      {
         result.push_back( api_witness_object( w ) );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, lookup_witness_accounts )
   {
      CHECK_ARG_SIZE( 2 )
      auto limit = args[1].as< uint32_t >();

      lookup_witness_accounts_return result;

      auto witnesses = _database_api->list_witnesses( { args[0], limit, database_api::by_name } ).witnesses;

      for( auto& w : witnesses )
      {
         result.push_back( w.owner );
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_witness_count )
   {
      CHECK_ARG_SIZE( 0 )
      return _db.get_index< witness_index >().indices().size();
   }

   DEFINE_API_IMPL( condenser_api_impl, get_transaction_hex )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->get_transaction_hex(
      {
         args[0].as< signed_transaction >()
      }).hex;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_transaction )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );

      return _account_history_api->get_transaction( { args[0].as< transaction_id_type >() } );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_required_signatures )
   {
      CHECK_ARG_SIZE( 2 )
      return _database_api->get_required_signatures( { args[0].as< signed_transaction >(), args[1].as< flat_set< public_key_type > >() } ).keys;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_potential_signatures )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->get_potential_signatures( { args[0].as< signed_transaction >() } ).keys;
   }

   DEFINE_API_IMPL( condenser_api_impl, verify_authority )
   {
      CHECK_ARG_SIZE( 1 )
      return _database_api->verify_authority( { args[0].as< signed_transaction >() } ).valid;
   }

   DEFINE_API_IMPL( condenser_api_impl, verify_account_authority )
   {
      CHECK_ARG_SIZE( 2 )
      return _database_api->verify_account_authority( { args[0].as< account_name_type >(), args[1].as< flat_set< public_key_type > >() } ).valid;
   }

   DEFINE_API_IMPL( condenser_api_impl, custom_object_subscription )
   {
      CHECK_ARG_SIZE( 5 )
      FC_ASSERT( _subscribe_api, "subscribe_api_plugin not enabled." );

      return _subscribe_api->custom_object_subscription( { args[0].as< uint64_t>(), args[1].as< uint32_t>(), args[2].as< string >(), args[3].as< string >(), args[4].as< uint64_t>() } );
   }


/*
   DEFINE_API_IMPL( condenser_api_impl, get_account_votes )
   {
      CHECK_ARG_SIZE( 1 )
      account_name_type voter = args[0].as< account_name_type >();

      vector< account_vote > result;

      const auto& voter_acnt = _db.get_account( voter );
      const auto& idx = _db.get_index< comment_vote_index, by_voter_comment >();

      account_id_type aid( voter_acnt.id );
      auto itr = idx.lower_bound( aid );
      auto end = idx.upper_bound( aid );
      while( itr != end )
      {
         const auto& vo = _db.get( itr->comment );
         account_vote avote;
         avote.authorperm = vo.author + "/" + to_string( vo.permlink );
         avote.weight = itr->weight;
         avote.rshares = itr->rshares;
         avote.percent = itr->vote_percent;
         avote.time = itr->last_update;
         result.push_back( avote );
         ++itr;
      }

      return result;
   }*/

   DEFINE_API_IMPL( condenser_api_impl, get_account_history )
   {
      CHECK_ARG_SIZE( 3 )
      FC_ASSERT( _account_history_api, "account_history_api_plugin not enabled." );

      auto history = _account_history_api->get_account_history( { args[0].as< account_name_type >(), args[1].as< uint64_t >(), args[2].as< uint32_t >() } ).history;
      get_account_history_return result;

      legacy_operation l_op;
      legacy_operation_conversion_visitor visitor( l_op );

      for( auto& entry : history )
      {
         if( entry.second.op.visit( visitor ) )
         {
            result.emplace( entry.first, api_operation_object( entry.second, visitor.l_op ) );
         }
      }

      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, list_received_documents )
   {
      CHECK_ARG_SIZE( 5 )
      FC_ASSERT( _custom_api, "custom_api_plugin not enabled." );

      return _custom_api->list_received_documents( { args[0].as< uint32_t >(), args[1].as< string >(), args[2].as< string >(), args[3].as< string >(), args[4].as< uint32_t >() } );
   }

   DEFINE_API_IMPL( condenser_api_impl, broadcast_transaction )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );

      return _network_broadcast_api->broadcast_transaction( { signed_transaction( args[0].as< legacy_signed_transaction >() ) } );
   }

   DEFINE_API_IMPL( condenser_api_impl, broadcast_transaction_synchronous )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );

      return _network_broadcast_api->broadcast_transaction_synchronous( { signed_transaction( args[0].as< legacy_signed_transaction >() ) } );
   }

   DEFINE_API_IMPL( condenser_api_impl, broadcast_block )
   {
      CHECK_ARG_SIZE( 1 )
      FC_ASSERT( _network_broadcast_api, "network_broadcast_api_plugin not enabled." );

      return _network_broadcast_api->broadcast_block( { signed_block( args[0].as< legacy_signed_block >() ) } );
   }

   DEFINE_API_IMPL( condenser_api_impl, get_applications )
   {
      CHECK_ARG_SIZE( 1 )
      vector< string > app_names = args[0].as< vector< string > >();

      vector< api_application_object > result;
      result.reserve( app_names.size() );

      for( auto& name : app_names )
      {
         auto itr = _db.find< application_object, by_name >( name );

         if( itr )
         {
            result.push_back( api_application_object( database_api::api_application_object( *itr ) ) );
         }
      }
      return result;
   }

   DEFINE_API_IMPL( condenser_api_impl, get_application_buyings )
   {
      CHECK_ARG_SIZE( 3 )
      FC_ASSERT( _database_api, "database_api_plugin not enabled." );

      return _database_api->get_application_buyings({ args[0], args[1].as< uint32_t >(), args[2].as< string >()}).application_buyings;
   }

  
   DEFINE_API_IMPL( condenser_api_impl, get_promotion_pool_balance )
   {
      CHECK_ARG_SIZE(0);
      return legacy_asset::from_asset(_database_api->get_promotion_pool_balance( {}  ));
   }

} // detail

condenser_api::condenser_api()
   : my( new detail::condenser_api_impl() )
{
   JSON_RPC_REGISTER_API( SOPHIATX_CONDENSER_API_PLUGIN_NAME );
   appbase::app().get_plugin< sophiatx::plugins::json_rpc::json_rpc_plugin >().add_api_subscribe_method("condenser_api", "custom_object_subscription" );
}

condenser_api::~condenser_api() {}

void condenser_api::api_startup()
{
   auto database = appbase::app().find_plugin< database_api::database_api_plugin >();
   if( database != nullptr )
      my->_database_api = database->api;

   auto block = appbase::app().find_plugin< block_api::block_api_plugin >();
   if( block != nullptr )
      my->_block_api = block->api;

   auto account_by_key = appbase::app().find_plugin< account_by_key::account_by_key_api_plugin >();
   if( account_by_key != nullptr )
      my->_account_by_key_api = account_by_key->api;

   auto account_history = appbase::app().find_plugin< account_history::account_history_api_plugin >();
   if( account_history != nullptr )
      my->_account_history_api = account_history->api;

   auto network_broadcast = appbase::app().find_plugin< network_broadcast_api::network_broadcast_api_plugin >();
   if( network_broadcast != nullptr )
      my->_network_broadcast_api = network_broadcast->api;

   auto witness = appbase::app().find_plugin< witness::witness_api_plugin >();
   if( witness != nullptr )
      my->_witness_api = witness->api;

   auto custom = appbase::app().find_plugin< custom::custom_api_plugin>();
   if( custom != nullptr )
      my->_custom_api = custom->api;

   auto subscribe = appbase::app().find_plugin< subscribe::subscribe_api_plugin>();
      my->_subscribe_api = subscribe->api;
}

DEFINE_LOCKLESS_APIS( condenser_api,
   (get_version)
   (get_config)
   (get_account_references)
   (broadcast_transaction)
   (broadcast_transaction_synchronous)
   (broadcast_block)
)

DEFINE_READ_APIS( condenser_api,
   (get_state)
   (get_active_witnesses)
   (get_block_header)
   (get_block)
   (get_ops_in_block)
   (get_dynamic_global_properties)
   (get_chain_properties)
   (get_current_median_history_price)
   (get_feed_history)
   (get_witness_schedule)
   (get_hardfork_version)
   (get_next_scheduled_hardfork)
   (get_key_references)
   (get_accounts)
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
   (custom_object_subscription)
 //  (get_account_votes)
   (get_account_history)
   (get_applications)
   (get_promotion_pool_balance)
   (list_received_documents)
   (get_application_buyings)
)

} } } // sophiatx::plugins::condenser_api
