#include <appbase/application.hpp>

#include <sophiatx/chain/database/database.hpp>

#include <sophiatx/plugins/database_api/database_api.hpp>
#include <sophiatx/plugins/database_api/database_api_plugin.hpp>

#include <sophiatx/protocol/get_config.hpp>
#include <sophiatx/protocol/exceptions.hpp>
#include <sophiatx/protocol/transaction_util.hpp>

namespace sophiatx { namespace plugins { namespace database_api {

class database_api_impl
{
   public:
      database_api_impl();
      ~database_api_impl();

      DECLARE_API_IMPL
      (
         (get_config)
         (get_dynamic_global_properties)
         (get_witness_schedule)
         (get_hardfork_properties)
         (get_current_price_feed)
         (get_feed_history)
         (list_witnesses)
         (find_witnesses)
         (list_witness_votes)
         (get_active_witnesses)
         (list_accounts)
         (find_accounts)
         (list_owner_histories)
         (find_owner_histories)
         (list_account_recovery_requests)
         (find_account_recovery_requests)
         (list_change_recovery_account_requests)
         (find_change_recovery_account_requests)
         (list_escrows)
         (find_escrows)
         (get_transaction_hex)
         (get_required_signatures)
         (get_potential_signatures)
         (verify_authority)
         (verify_account_authority)
         (verify_signatures)
         (list_applications)
         (get_application_buyings)
         (get_promotion_pool_balance)
         (get_burned_balance)
      )

      template< typename ResultType >
      static ResultType on_push_default( const ResultType& r ) { return r; }

      template< typename IndexType, typename OrderType, typename ValueType, typename ResultType, typename OnPush >
      void iterate_results( ValueType start, vector< ResultType >& result, uint32_t limit, OnPush&& on_push = &database_api_impl::on_push_default< ResultType >, bool reverse = false )
      {
         const auto& idx = _db->get_index< IndexType, OrderType >();
         auto itr = reverse? (idx.upper_bound(start)): idx.lower_bound( start );
         auto end = reverse? idx.begin() : idx.end();

         while( result.size() < limit && itr != end )
         {
            if (reverse) --itr;
            result.push_back( on_push( *itr ) );
            if (!reverse) ++itr;
         }
      }



      std::shared_ptr<database> _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api()
   : my( new database_api_impl() )
{
   JSON_RPC_REGISTER_API( SOPHIATX_DATABASE_API_PLUGIN_NAME );
}

database_api::~database_api() {}

database_api_impl::database_api_impl()
   : _db( std::static_pointer_cast<database>(appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db()) ) {}

database_api_impl::~database_api_impl() {}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////


DEFINE_API_IMPL( database_api_impl, get_config )
{
   return sophiatx::protocol::get_config();
}

DEFINE_API_IMPL( database_api_impl, get_dynamic_global_properties )
{
   return _db->get_dynamic_global_properties();
}

DEFINE_API_IMPL( database_api_impl, get_witness_schedule )
{
   return api_witness_schedule_object( _db->get_witness_schedule_object() );
}

DEFINE_API_IMPL( database_api_impl, get_hardfork_properties )
{
   return _db->get_hardfork_property_object();
}

DEFINE_API_IMPL( database_api_impl, get_current_price_feed )
{
   return _db->get_feed_history( args.symbol ).current_median_history;;
}

DEFINE_API_IMPL( database_api_impl, get_feed_history )
{
   return _db->get_feed_history( args.symbol );
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API_IMPL( database_api_impl, list_witnesses )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   list_witnesses_return result;
   result.witnesses.reserve( args.limit );
   bool reverse = (args.order >= by_name_reverse );

   switch( args.order )
   {
      case( by_name ):
      case( by_name_reverse ):
      {
         iterate_results< chain::witness_index, chain::by_name >(
            args.start.as< protocol::account_name_type >(),
            result.witnesses,
            args.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); },
            reverse);
         break;
      }
      case( by_id ):
      case( by_id_reverse ):
      {
         iterate_results< chain::witness_index, chain::by_id >(
               args.start.as< witness_id_type >(),
               result.witnesses,
               args.limit,
               [&]( const witness_object& w ){ return api_witness_object( w ); },
               reverse);
         break;
      }
      case( by_vote_name ):
      case( by_vote_name_reverse ):
      {
         auto key = args.start.as< std::pair< share_type, account_name_type > >();
         iterate_results< chain::witness_index, chain::by_vote_name >(
            boost::make_tuple( key.first, key.second ),
            result.witnesses,
            args.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); },
            reverse );
         break;
      }
      case( by_schedule_time ):
      case( by_schedule_time_reverse ):
      {
         auto key = args.start.as< std::pair< fc::uint128, account_name_type > >();
         auto wit_id = _db->get< chain::witness_object, chain::by_name >( key.second ).id;
         iterate_results< chain::witness_index, chain::by_schedule_time >(
            boost::make_tuple( key.first, wit_id ),
            result.witnesses,
            args.limit,
            [&]( const witness_object& w ){ return api_witness_object( w ); },
            reverse );
         break;
      }

      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_witnesses )
{
   FC_ASSERT( args.owners.size() <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   find_witnesses_return result;

   for( auto& o : args.owners )
   {
      auto witness = _db->find< chain::witness_object, chain::by_name >( o );

      if( witness != nullptr )
         result.witnesses.push_back( api_witness_object( *witness ) );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, list_witness_votes )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   list_witness_votes_return result;
   result.votes.reserve( args.limit );
   bool reverse = (args.order >= by_name_reverse );

   switch( args.order )
   {
      case( by_account_witness ):
      case( by_account_witness_reverse ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::witness_vote_index, chain::by_account_witness >(
            boost::make_tuple( key.first, key.second ),
            result.votes,
            args.limit,
            [&]( const witness_vote_object& v ){ return api_witness_vote_object( v ); },
            reverse );
         break;
      }
      case( by_witness_account ):
      case( by_witness_account_reverse ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::witness_vote_index, chain::by_witness_account >(
            boost::make_tuple( key.first, key.second ),
            result.votes,
            args.limit,
            [&]( const witness_vote_object& v ){ return api_witness_vote_object( v ); },
            reverse );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, get_active_witnesses )
{
   const auto& wso = _db->get_witness_schedule_object();
   size_t n = wso.current_shuffled_witnesses.size();
   get_active_witnesses_return result;
   result.witnesses.reserve( n );
   for( size_t i=0; i<n; i++ )
      result.witnesses.push_back( wso.current_shuffled_witnesses[i] );
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

/* Accounts */

DEFINE_API_IMPL( database_api_impl, list_accounts )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   list_accounts_return result;
   result.accounts.reserve( args.limit );
   bool reverse = (args.order >= by_name_reverse );

   switch( args.order )
   {
      case( by_name ):
      case( by_name_reverse ):
      {
         iterate_results< chain::account_index, chain::by_name >(
            args.start.as< protocol::account_name_type >(),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); }, reverse );
         break;
      }
      case( by_id ):
      {
         iterate_results< chain::account_index, chain::by_id >(
            args.start.as< account_id_type >(),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); } );
         break;
      }
      case( by_proxy ):
      case( by_proxy_reverse ):
      {
         auto key = args.start.as< std::pair< account_name_type, account_name_type > >();
         iterate_results< chain::account_index, chain::by_proxy >(
            boost::make_tuple( key.first, key.second ),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); }, reverse );
         break;
      }
      case( by_next_vesting_withdrawal ):
      case( by_next_vesting_withdrawal_reverse ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::account_index, chain::by_next_vesting_withdrawal >(
            boost::make_tuple( key.first, key.second ),
            result.accounts,
            args.limit,
            [&]( const account_object& a ){ return api_account_object( a, _db ); }, reverse );
         break;
      }
      case( by_balance ):
      {
         iterate_results< chain::account_index, chain::by_balance >(
               args.start.as< protocol::share_type >(),
               result.accounts,
               args.limit,
               [&]( const account_object& a ){ return api_account_object( a, _db ); }, true );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_accounts )
{
   find_accounts_return result;
   FC_ASSERT( args.accounts.size() <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto acct = _db->find< chain::account_object, chain::by_name >( a );
      if( acct != nullptr )
         result.accounts.push_back( api_account_object( *acct, _db ) );
   }

   return result;
}


/* Owner Auth Histories */

DEFINE_API_IMPL( database_api_impl, list_owner_histories )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   list_owner_histories_return result;
   result.owner_auths.reserve( args.limit );

   auto key = args.start.as< std::pair< account_name_type, fc::time_point_sec > >();
   iterate_results< chain::owner_authority_history_index, chain::by_account >(
      boost::make_tuple( key.first, key.second ),
      result.owner_auths,
      args.limit,
      [&]( const owner_authority_history_object& o ){ return api_owner_authority_history_object( o ); }, false );

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_owner_histories )
{
   find_owner_histories_return result;

   const auto& hist_idx = _db->get_index< chain::owner_authority_history_index, chain::by_account >();
   auto itr = hist_idx.lower_bound( args.owner );

   while( itr != hist_idx.end() && itr->account == args.owner && result.owner_auths.size() <= SOPHIATX_API_SINGLE_QUERY_LIMIT )
   {
      result.owner_auths.push_back( api_owner_authority_history_object( *itr ) );
      ++itr;
   }

   return result;
}


/* Account Recovery Requests */

DEFINE_API_IMPL( database_api_impl, list_account_recovery_requests )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   list_account_recovery_requests_return result;
   result.requests.reserve( args.limit );
   bool reverse = (args.order >= by_name_reverse );

   switch( args.order )
   {
      case( by_account ):
      case( by_account_reverse ):
      {
         iterate_results< chain::account_recovery_request_index, chain::by_account >(
            args.start.as< account_name_type >(),
            result.requests,
            args.limit,
            [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); }, reverse );
         break;
      }
      case( by_expiration ):
      case( by_expiration_reverse ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::account_recovery_request_index, chain::by_expiration >(
            boost::make_tuple( key.first, key.second ),
            result.requests,
            args.limit,
            [&]( const account_recovery_request_object& a ){ return api_account_recovery_request_object( a ); }, reverse );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_account_recovery_requests )
{
   find_account_recovery_requests_return result;
   FC_ASSERT( args.accounts.size() <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto request = _db->find< chain::account_recovery_request_object, chain::by_account >( a );

      if( request != nullptr )
         result.requests.push_back( api_account_recovery_request_object( *request ) );
   }

   return result;
}


/* Change Recovery Account Requests */

DEFINE_API_IMPL( database_api_impl, list_change_recovery_account_requests )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   list_change_recovery_account_requests_return result;
   result.requests.reserve( args.limit );
   bool reverse = (args.order >= by_name_reverse );

   switch( args.order )
   {
      case( by_account ):
      case( by_account_reverse ):
      {
         iterate_results< chain::change_recovery_account_request_index, chain::by_account >(
            args.start.as< account_name_type >(),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< change_recovery_account_request_object >, reverse );
         break;
      }
      case( by_effective_date ):
      case( by_effective_date_reverse ):
      {
         auto key = args.start.as< std::pair< fc::time_point_sec, account_name_type > >();
         iterate_results< chain::change_recovery_account_request_index, chain::by_effective_date >(
            boost::make_tuple( key.first, key.second ),
            result.requests,
            args.limit,
            &database_api_impl::on_push_default< change_recovery_account_request_object >, reverse );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_change_recovery_account_requests )
{
   find_change_recovery_account_requests_return result;
   FC_ASSERT( args.accounts.size() <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   for( auto& a : args.accounts )
   {
      auto request = _db->find< chain::change_recovery_account_request_object, chain::by_account >( a );

      if( request != nullptr )
         result.requests.push_back( *request );
   }

   return result;
}


/* Escrows */

DEFINE_API_IMPL( database_api_impl, list_escrows )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   list_escrows_return result;
   result.escrows.reserve( args.limit );
   bool reverse = (args.order >= by_name_reverse );

   switch( args.order )
   {
      case( by_from_id ):
      case( by_from_id_reverse ):
      {
         auto key = args.start.as< std::pair< account_name_type, uint32_t > >();
         iterate_results< chain::escrow_index, chain::by_from_id >(
            boost::make_tuple( key.first, key.second ),
            result.escrows,
            args.limit,
            &database_api_impl::on_push_default< escrow_object >, reverse );
         break;
      }
      case( by_ratification_deadline ):
      case( by_ratification_deadline_reverse ):
      {
         auto key = args.start.as< std::vector< fc::variant > >();
         FC_ASSERT( key.size() == 3, "by_ratification_deadline start requires 3 values. (bool, time_point_sec, escrow_id_type)" );
         iterate_results< chain::escrow_index, chain::by_ratification_deadline >(
            boost::make_tuple( key[0].as< bool >(), key[1].as< fc::time_point_sec >(), key[2].as< escrow_id_type >() ),
            result.escrows,
            args.limit,
            &database_api_impl::on_push_default< escrow_object >, reverse );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, find_escrows )
{
   find_escrows_return result;

   const auto& escrow_idx = _db->get_index< chain::escrow_index, chain::by_from_id >();
   auto itr = escrow_idx.lower_bound( args.from );

   while( itr != escrow_idx.end() && itr->from == args.from && result.escrows.size() <= SOPHIATX_API_SINGLE_QUERY_LIMIT )
   {
      result.escrows.push_back( *itr );
      ++itr;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Applications                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API_IMPL( database_api_impl, list_applications )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   list_applications_return result;
   result.applications.reserve( args.limit );
   bool reverse = (args.order >= by_name_reverse );

   switch( args.order )
   {
      case( by_name ):
      case( by_name_reverse ):
      {
         iterate_results< chain::application_index, chain::by_name >(
                 args.start.as<string>(),
                 result.applications,
                 args.limit,
                 [&]( const application_object& a ){ return api_application_object( a ); }, reverse );
         break;
      }
      case( by_author ):
      case( by_author_reverse ):
      {
         iterate_results< chain::application_index, chain::by_author >(
                 args.start.as< protocol::account_name_type >(),
                 result.applications,
                 args.limit,
                 [&]( const application_object& a ){ return api_application_object( a ); }, reverse );
         break;
      }
      default:
         FC_ASSERT( false, "Unknown or unsupported sort order" );
   }

   return result;
}

DEFINE_API_IMPL( database_api_impl, get_application_buyings )
{
   FC_ASSERT( args.limit <= SOPHIATX_API_SINGLE_QUERY_LIMIT );

   get_application_buyings_return result;
   result.application_buyings.reserve( args.limit );

   if(args.search_type == "by_buyer") {
       iterate_results< chain::application_buying_index, chain::by_author >(
               args.start.as<account_name_type>(),
               result.application_buyings,
               args.limit,
               [&]( const application_buying_object& a ){ return api_application_buying_object( a ); }, true );
   } else if(args.search_type == "by_app_id") {
       iterate_results< chain::application_buying_index, chain::by_app_id >(
               args.start.as<application_id_type>(),
               result.application_buyings,
               args.limit,
               [&]( const application_buying_object& a ){ return api_application_buying_object( a ); }, true );
   } else {
      FC_ASSERT( false, "Unknown search type argument" );
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / Validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API_IMPL( database_api_impl, get_transaction_hex )
{
   return get_transaction_hex_return( { fc::to_hex( fc::raw::pack_to_vector( args.trx ) ) } );
}

DEFINE_API_IMPL( database_api_impl, get_required_signatures )
{
   get_required_signatures_return result;
   result.keys = args.trx.get_required_signatures( _db->get_chain_id(),
                                                   args.available_keys,
                                                   [&]( string account_name ){ return authority( _db->get< chain::account_authority_object, chain::by_account >( account_name ).active  ); },
                                                   [&]( string account_name ){ return authority( _db->get< chain::account_authority_object, chain::by_account >( account_name ).owner   ); },
                                                   SOPHIATX_MAX_SIG_CHECK_DEPTH,
                                                   _db->has_hardfork( SOPHIATX_HARDFORK_1_1 ) ? fc::ecc::canonical_signature_type::bip_0062 : fc::ecc::canonical_signature_type::fc_canonical);

   return result;
}

DEFINE_API_IMPL( database_api_impl, get_potential_signatures )
{
   get_potential_signatures_return result;
   args.trx.get_required_signatures(
      _db->get_chain_id(),
      flat_set< public_key_type >(),
      [&]( account_name_type account_name )
      {
         const auto& auth = _db->get< chain::account_authority_object, chain::by_account >( account_name ).active;
         for( const auto& k : auth.get_keys() )
            result.keys.insert( k );
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db->get< chain::account_authority_object, chain::by_account >( account_name ).owner;
         for( const auto& k : auth.get_keys() )
            result.keys.insert( k );
         return authority( auth );
      },
      SOPHIATX_MAX_SIG_CHECK_DEPTH,
      _db->has_hardfork( SOPHIATX_HARDFORK_1_1 ) ? fc::ecc::canonical_signature_type::bip_0062 : fc::ecc::canonical_signature_type::fc_canonical
   );

   return result;
}

DEFINE_API_IMPL( database_api_impl, verify_authority )
{
   args.trx.verify_authority(_db->get_chain_id(),
                           [&]( string account_name ){ return authority( _db->get< chain::account_authority_object, chain::by_account >( account_name ).active  ); },
                           [&]( string account_name ){ return authority( _db->get< chain::account_authority_object, chain::by_account >( account_name ).owner   ); },
                           SOPHIATX_MAX_SIG_CHECK_DEPTH,
                           _db->has_hardfork( SOPHIATX_HARDFORK_1_1 ) ? fc::ecc::canonical_signature_type::bip_0062 : fc::ecc::canonical_signature_type::fc_canonical);
   return verify_authority_return( { true } );
}

// TODO: This is broken. By the look of is, it has been since BitShares. verify_authority always
// returns false because the TX is not signed.
DEFINE_API_IMPL( database_api_impl, verify_account_authority )
{
   auto account = _db->find< chain::account_object, chain::by_name >( args.account );
   FC_ASSERT( account != nullptr, "no such account" );

   /// reuse trx.verify_authority by creating a dummy transfer
   verify_authority_args vap;
   transfer_operation op;
   op.from = account->name;
   vap.trx.operations.emplace_back( op );

   return verify_authority( vap );
}

DEFINE_API_IMPL( database_api_impl, verify_signatures )
{
   // get_signature_keys can throw for dup sigs. Allow this to throw.
   flat_set< public_key_type > sig_keys;
   for( const auto&  sig : args.signatures )
   {
      SOPHIATX_ASSERT(
         sig_keys.insert( fc::ecc::public_key::recover_key( sig, args.hash ) ).second,
         protocol::tx_duplicate_sig,
         "Duplicate Signature detected" );
   }

   verify_signatures_return result;
   result.valid = true;

   // verify authority throws on failure, catch and return false
   try
   {
      sophiatx::protocol::verify_authority< verify_signatures_args >(
         { args },
         sig_keys,
         [this]( const string& name ) { return authority( _db->get< chain::account_authority_object, chain::by_account >( name ).owner ); },
         [this]( const string& name ) { return authority( _db->get< chain::account_authority_object, chain::by_account >( name ).active ); },
         SOPHIATX_MAX_SIG_CHECK_DEPTH );
   }
   catch( fc::exception& ) { result.valid = false; }

   return result;
}

DEFINE_API_IMPL( database_api_impl, get_promotion_pool_balance )
{
   return asset(_db->get_economic_model().get_available_promotion_pool(_db->head_block_num()), SOPHIATX_SYMBOL);
}


DEFINE_API_IMPL( database_api_impl, get_burned_balance )
{
   return asset(_db->get_economic_model().burn_pool, SOPHIATX_SYMBOL);
}

#ifdef SOPHIATX_ENABLE_SMT
//////////////////////////////////////////////////////////////////////
//                                                                  //
// SMT                                                              //
//                                                                  //
//////////////////////////////////////////////////////////////////////

DEFINE_API_IMPL( database_api_impl, get_smt_next_identifier )
{
   get_smt_next_identifier_return result;
   result.nais = _db->get_smt_next_identifier();
   return result;
}
#endif

DEFINE_LOCKLESS_APIS( database_api, (get_config) )

DEFINE_READ_APIS( database_api,
   (get_dynamic_global_properties)
   (get_witness_schedule)
   (get_hardfork_properties)
   (get_current_price_feed)
   (get_feed_history)
   (list_witnesses)
   (find_witnesses)
   (list_witness_votes)
   (get_active_witnesses)
   (list_accounts)
   (find_accounts)
   (list_owner_histories)
   (find_owner_histories)
   (list_account_recovery_requests)
   (find_account_recovery_requests)
   (list_change_recovery_account_requests)
   (find_change_recovery_account_requests)
   (list_escrows)
   (find_escrows)
   (get_transaction_hex)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)
   (verify_signatures)
   (list_applications)
   (get_application_buyings)
   (get_promotion_pool_balance)
   (get_burned_balance)
)

} } } // sophiatx::plugins::database_api
