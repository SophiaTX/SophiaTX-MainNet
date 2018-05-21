#include <steem/chain/steem_evaluator.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/custom_operation_interpreter.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/witness_objects.hpp>
#include <steem/chain/block_summary_object.hpp>

#include <fc/macros.hpp>

#ifndef IS_LOW_MEM
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <diff_match_patch.h>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#endif

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>
#include <steem/chain/application_object.hpp>

namespace steem { namespace chain {
   using fc::uint128_t;

struct strcmp_equal
{
   bool operator()( const shared_string& a, const string& b )
   {
      return a.size() == b.size() || std::strcmp( a.c_str(), b.c_str() ) == 0;
   }
};

template< bool force_canon >
void copy_legacy_chain_properties( chain_properties& dest, const legacy_chain_properties& src )
{
   dest.account_creation_fee = src.account_creation_fee;
   dest.maximum_block_size = src.maximum_block_size;
}

void witness_update_evaluator::do_apply( const witness_update_operation& o )
{
   _db.get_account( o.owner ); // verify owner exists

   if( _db.is_producing() )
   {
      FC_ASSERT( o.props.maximum_block_size <= STEEM_SOFT_MAX_BLOCK_SIZE, "Max block size cannot be more than 2MiB" );
   }

   if( _db.is_producing() )
   {
      FC_ASSERT( o.props.maximum_block_size <= STEEM_SOFT_MAX_BLOCK_SIZE, "Max block size cannot be more than 2MiB" );
   }

   const auto& by_witness_name_idx = _db.get_index< witness_index >().indices().get< by_name >();
   auto wit_itr = by_witness_name_idx.find( o.owner );
   if( wit_itr != by_witness_name_idx.end() )
   {
      _db.modify( *wit_itr, [&]( witness_object& w ) {
         from_string( w.url, o.url );
         w.signing_key        = o.block_signing_key;
         copy_legacy_chain_properties< false >( w.props, o.props );
      });
   }
   else
   {
      _db.create< witness_object >( [&]( witness_object& w ) {
         w.owner              = o.owner;
         from_string( w.url, o.url );
         w.signing_key        = o.block_signing_key;
         w.created            = _db.head_block_time();
         copy_legacy_chain_properties< false >( w.props, o.props );
      });
   }
}

void witness_set_properties_evaluator::do_apply( const witness_set_properties_operation& o )
{
   const auto& witness = _db.get< witness_object, by_name >( o.owner ); // verifies witness exists;

   // Capture old properties. This allows only updating the object once.
   chain_properties  props;
   public_key_type   signing_key;
   price             sbd_exchange_rate;
   time_point_sec    last_sbd_exchange_update;
   string            url;

   bool account_creation_changed = false;
   bool max_block_changed        = false;
   bool key_changed              = false;
   bool sbd_exchange_changed     = false;
   bool url_changed              = false;

   auto itr = o.props.find( "key" );

   // This existence of 'key' is checked in witness_set_properties_operation::validate
   fc::raw::unpack_from_vector( itr->second, signing_key );
   FC_ASSERT( signing_key == witness.signing_key, "'key' does not match witness signing key.",
      ("key", signing_key)("signing_key", witness.signing_key) );

   itr = o.props.find( "account_creation_fee" );
   if( itr != o.props.end() )
   {
      fc::raw::unpack_from_vector( itr->second, props.account_creation_fee );
      account_creation_changed = true;
   }

   itr = o.props.find( "maximum_block_size" );
   if( itr != o.props.end() )
   {
      fc::raw::unpack_from_vector( itr->second, props.maximum_block_size );
      max_block_changed = true;
   }

   itr = o.props.find( "new_signing_key" );
   if( itr != o.props.end() )
   {
      fc::raw::unpack_from_vector( itr->second, signing_key );
      key_changed = true;
   }

   itr = o.props.find( "sbd_exchange_rate" );
   if( itr != o.props.end() )
   {
      fc::raw::unpack_from_vector( itr->second, sbd_exchange_rate );
      last_sbd_exchange_update = _db.head_block_time();
      sbd_exchange_changed = true;
   }

   itr = o.props.find( "url" );
   if( itr != o.props.end() )
   {
      fc::raw::unpack_from_vector< std::string >( itr->second, url );
      url_changed = true;
   }

   _db.modify( witness, [&]( witness_object& w )
   {
      if( account_creation_changed )
         w.props.account_creation_fee = props.account_creation_fee;

      if( max_block_changed )
         w.props.maximum_block_size = props.maximum_block_size;

      if( key_changed )
         w.signing_key = signing_key;

      if( sbd_exchange_changed )
      {
         w.sbd_exchange_rate = sbd_exchange_rate;
         w.last_sbd_exchange_update = last_sbd_exchange_update;
      }

      if( url_changed )
         from_string( w.url, url );
   });
}

void verify_authority_accounts_exist(
   const database& db,
   const authority& auth,
   const account_name_type& auth_account,
   authority::classification auth_class)
{
   for( const std::pair< account_name_type, weight_type >& aw : auth.account_auths )
   {
      const account_object* a = db.find_account( aw.first );
      FC_ASSERT( a != nullptr, "New ${ac} authority on account ${aa} references non-existing account ${aref}",
         ("aref", aw.first)("ac", auth_class)("aa", auth_account) );
   }
}

void account_create_evaluator::do_apply( const account_create_operation& o )
{
   const auto& creator = _db.get_account( o.creator );

   const auto& props = _db.get_dynamic_global_properties();

   FC_ASSERT( creator.balance >= o.fee, "Insufficient balance to create account.", ( "creator.balance", creator.balance )( "required", o.fee ) );


   const witness_schedule_object& wso = _db.get_witness_schedule_object();
   FC_ASSERT( o.fee >= asset( wso.median_props.account_creation_fee.amount * STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER, STEEM_SYMBOL ), "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", wso.median_props.account_creation_fee * asset( STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER, STEEM_SYMBOL ) )
                    ("p", o.fee) );

   verify_authority_accounts_exist( _db, o.owner, o.new_account_name, authority::owner );
   verify_authority_accounts_exist( _db, o.active, o.new_account_name, authority::active );


   const auto& new_account = _db.create< account_object >( [&]( account_object& acc )
   {
      acc.name = o.new_account_name;
      acc.memo_key = o.memo_key;
      acc.created = props.time;
      acc.mined = false;

      acc.recovery_account = o.creator;

      #ifndef IS_LOW_MEM
         from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   _db.create< account_authority_object >( [&]( account_authority_object& auth )
   {
      auth.account = o.new_account_name;
      auth.owner = o.owner;
      auth.active = o.active;
      auth.last_owner_update = fc::time_point_sec::min();
   });

   if( o.fee.amount > 0 )
      _db.pay_fee( creator, o.fee );
}


void account_update_evaluator::do_apply( const account_update_operation& o )
{
   FC_ASSERT( o.account != STEEM_TEMP_ACCOUNT, "Cannot update temp account." );

   const auto& account = _db.get_account( o.account );
   const auto& account_auth = _db.get< account_authority_object, by_account >( o.account );

   if( o.owner )
   {
#ifndef IS_TEST_NET
      FC_ASSERT( _db.head_block_time() - account_auth.last_owner_update > STEEM_OWNER_UPDATE_LIMIT, "Owner authority can only be updated once an hour." );
#endif

      verify_authority_accounts_exist( _db, *o.owner, o.account, authority::owner );

      _db.update_owner_authority( account, *o.owner );
   }
   if( o.active  )
      verify_authority_accounts_exist( _db, *o.active, o.account, authority::active );

   _db.modify( account, [&]( account_object& acc )
   {
      if( o.memo_key != public_key_type() )
            acc.memo_key = o.memo_key;

      acc.last_account_update = _db.head_block_time();

      #ifndef IS_LOW_MEM
        if ( o.json_metadata.size() > 0 )
            from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   if( o.active )
   {
      _db.modify( account_auth, [&]( account_authority_object& auth)
      {
         if( o.active )  auth.active  = *o.active;
      });
   }

}

void escrow_transfer_evaluator::do_apply( const escrow_transfer_operation& o )
{
   try
   {
      const auto& from_account = _db.get_account(o.from);
      _db.get_account(o.to);
      _db.get_account(o.agent);

      FC_ASSERT( o.ratification_deadline > _db.head_block_time(), "The escorw ratification deadline must be after head block time." );
      FC_ASSERT( o.escrow_expiration > _db.head_block_time(), "The escrow expiration must be after head block time." );

      asset steem_spent = o.steem_amount;
      if( o.fee.symbol == STEEM_SYMBOL )
         steem_spent += o.fee;


      FC_ASSERT( from_account.balance >= steem_spent, "Account cannot cover STEEM costs of escrow. Required: ${r} Available: ${a}", ("r",steem_spent)("a",from_account.balance) );

      _db.adjust_balance( from_account, -steem_spent );

      _db.create<escrow_object>([&]( escrow_object& esc )
      {
         esc.escrow_id              = o.escrow_id;
         esc.from                   = o.from;
         esc.to                     = o.to;
         esc.agent                  = o.agent;
         esc.ratification_deadline  = o.ratification_deadline;
         esc.escrow_expiration      = o.escrow_expiration;
         esc.steem_balance          = o.steem_amount;
         esc.pending_fee            = o.fee;
      });
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_approve_evaluator::do_apply( const escrow_approve_operation& o )
{
   try
   {

      const auto& escrow = _db.get_escrow( o.from, o.escrow_id );

      FC_ASSERT( escrow.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).", ("o", o.to)("e", escrow.to) );
      FC_ASSERT( escrow.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).", ("o", o.agent)("e", escrow.agent) );
      FC_ASSERT( escrow.ratification_deadline >= _db.head_block_time(), "The escrow ratification deadline has passed. Escrow can no longer be ratified." );

      bool reject_escrow = !o.approve;

      if( o.who == o.to )
      {
         FC_ASSERT( !escrow.to_approved, "Account 'to' (${t}) has already approved the escrow.", ("t", o.to) );

         if( !reject_escrow )
         {
            _db.modify( escrow, [&]( escrow_object& esc )
            {
               esc.to_approved = true;
            });
         }
      }
      if( o.who == o.agent )
      {
         FC_ASSERT( !escrow.agent_approved, "Account 'agent' (${a}) has already approved the escrow.", ("a", o.agent) );

         if( !reject_escrow )
         {
            _db.modify( escrow, [&]( escrow_object& esc )
            {
               esc.agent_approved = true;
            });
         }
      }

      if( reject_escrow )
      {
         _db.adjust_balance( o.from, escrow.steem_balance );
         _db.adjust_balance( o.from, escrow.pending_fee );

         _db.remove( escrow );
      }
      else if( escrow.to_approved && escrow.agent_approved )
      {
         _db.adjust_balance( o.agent, escrow.pending_fee );

         _db.modify( escrow, [&]( escrow_object& esc )
         {
            esc.pending_fee.amount = 0;
         });
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_dispute_evaluator::do_apply( const escrow_dispute_operation& o )
{
   try
   {
      _db.get_account( o.from ); // Verify from account exists

      const auto& e = _db.get_escrow( o.from, o.escrow_id );
      FC_ASSERT( _db.head_block_time() < e.escrow_expiration, "Disputing the escrow must happen before expiration." );
      FC_ASSERT( e.to_approved && e.agent_approved, "The escrow must be approved by all parties before a dispute can be raised." );
      FC_ASSERT( !e.disputed, "The escrow is already under dispute." );
      FC_ASSERT( e.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).", ("o", o.to)("e", e.to) );
      FC_ASSERT( e.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).", ("o", o.agent)("e", e.agent) );

      _db.modify( e, [&]( escrow_object& esc )
      {
         esc.disputed = true;
      });
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_release_evaluator::do_apply( const escrow_release_operation& o )
{
   try
   {
      _db.get_account(o.from); // Verify from account exists

      const auto& e = _db.get_escrow( o.from, o.escrow_id );
      FC_ASSERT( e.steem_balance >= o.steem_amount, "Release amount exceeds escrow balance. Amount: ${a}, Balance: ${b}", ("a", o.steem_amount)("b", e.steem_balance) );
      FC_ASSERT( e.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).", ("o", o.to)("e", e.to) );
      FC_ASSERT( e.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).", ("o", o.agent)("e", e.agent) );
      FC_ASSERT( o.receiver == e.from || o.receiver == e.to, "Funds must be released to 'from' (${f}) or 'to' (${t})", ("f", e.from)("t", e.to) );
      FC_ASSERT( e.to_approved && e.agent_approved, "Funds cannot be released prior to escrow approval." );

      // If there is a dispute regardless of expiration, the agent can release funds to either party
      if( e.disputed )
      {
         FC_ASSERT( o.who == e.agent, "Only 'agent' (${a}) can release funds in a disputed escrow.", ("a", e.agent) );
      }
      else
      {
         FC_ASSERT( o.who == e.from || o.who == e.to, "Only 'from' (${f}) and 'to' (${t}) can release funds from a non-disputed escrow", ("f", e.from)("t", e.to) );

         if( e.escrow_expiration > _db.head_block_time() )
         {
            // If there is no dispute and escrow has not expired, either party can release funds to the other.
            if( o.who == e.from )
            {
               FC_ASSERT( o.receiver == e.to, "Only 'from' (${f}) can release funds to 'to' (${t}).", ("f", e.from)("t", e.to) );
            }
            else if( o.who == e.to )
            {
               FC_ASSERT( o.receiver == e.from, "Only 'to' (${t}) can release funds to 'from' (${t}).", ("f", e.from)("t", e.to) );
            }
         }
      }
      // If escrow expires and there is no dispute, either party can release funds to either party.

      _db.adjust_balance( o.receiver, o.steem_amount );

      _db.modify( e, [&]( escrow_object& esc )
      {
         esc.steem_balance -= o.steem_amount;
      });

      if( e.steem_balance.amount == 0 )
      {
         _db.remove( e );
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void transfer_evaluator::do_apply( const transfer_operation& o )
{
   FC_ASSERT( _db.get_balance( o.from, o.amount.symbol ) >= o.amount, "Account does not have sufficient funds for transfer." );
   _db.adjust_balance( o.from, -o.amount );
   _db.adjust_balance( o.to, o.amount );
}

void transfer_to_vesting_evaluator::do_apply( const transfer_to_vesting_operation& o )
{
   const auto& from_account = _db.get_account(o.from);
   const auto& to_account = o.to.size() ? _db.get_account(o.to) : from_account;

   FC_ASSERT( _db.get_balance( from_account, STEEM_SYMBOL) >= o.amount, "Account does not have sufficient STEEM for transfer." );
   _db.adjust_balance( from_account, -o.amount );
   _db.create_vesting( to_account, o.amount );
}

void withdraw_vesting_evaluator::do_apply( const withdraw_vesting_operation& o )
{
   const auto& account = _db.get_account( o.account );

   FC_ASSERT( account.vesting_shares >= asset( 0, VESTS_SYMBOL ), "Account does not have sufficient Steem Power for withdraw." );
   FC_ASSERT( account.vesting_shares >= o.vesting_shares, "Account does not have sufficient Steem Power for withdraw." );


   if( o.vesting_shares.amount == 0 )
   {
      FC_ASSERT( account.vesting_withdraw_rate.amount  != 0, "This operation would not change the vesting withdraw rate." );

      _db.modify( account, [&]( account_object& a ) {
         a.vesting_withdraw_rate = asset( 0, VESTS_SYMBOL );
         a.next_vesting_withdrawal = time_point_sec::maximum();
         a.to_withdraw = 0;
         a.withdrawn = 0;
      });
   }
   else
   {
      int vesting_withdraw_intervals = STEEM_VESTING_WITHDRAW_INTERVALS; /// 13 weeks = 1 quarter of a year

      _db.modify( account, [&]( account_object& a )
      {
         auto new_vesting_withdraw_rate = asset( o.vesting_shares.amount / vesting_withdraw_intervals, VESTS_SYMBOL );

         if( new_vesting_withdraw_rate.amount == 0 )
            new_vesting_withdraw_rate.amount = 1;

         FC_ASSERT( account.vesting_withdraw_rate  != new_vesting_withdraw_rate, "This operation would not change the vesting withdraw rate." );

         a.vesting_withdraw_rate = new_vesting_withdraw_rate;
         a.next_vesting_withdrawal = _db.head_block_time() + fc::seconds(STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS);
         a.to_withdraw = o.vesting_shares.amount;
         a.withdrawn = 0;
      });
   }
}


void account_witness_proxy_evaluator::do_apply( const account_witness_proxy_operation& o )
{
   const auto& account = _db.get_account( o.account );
   FC_ASSERT( account.proxy != o.proxy, "Proxy must change." );

   /// remove all current votes
   std::array<share_type, STEEM_MAX_PROXY_RECURSION_DEPTH+1> delta;
   delta[0] = -account.vesting_shares.amount;
   for( int i = 0; i < STEEM_MAX_PROXY_RECURSION_DEPTH; ++i )
      delta[i+1] = -account.proxied_vsf_votes[i];
   _db.adjust_proxied_witness_votes( account, delta );

   if( o.proxy.size() ) {
      const auto& new_proxy = _db.get_account( o.proxy );
      flat_set<account_id_type> proxy_chain( { account.id, new_proxy.id } );
      proxy_chain.reserve( STEEM_MAX_PROXY_RECURSION_DEPTH + 1 );

      /// check for proxy loops and fail to update the proxy if it would create a loop
      auto cprox = &new_proxy;
      while( cprox->proxy.size() != 0 ) {
         const auto next_proxy = _db.get_account( cprox->proxy );
         FC_ASSERT( proxy_chain.insert( next_proxy.id ).second, "This proxy would create a proxy loop." );
         cprox = &next_proxy;
         FC_ASSERT( proxy_chain.size() <= STEEM_MAX_PROXY_RECURSION_DEPTH, "Proxy chain is too long." );
      }

      /// clear all individual vote records
      _db.clear_witness_votes( account );

      _db.modify( account, [&]( account_object& a ) {
         a.proxy = o.proxy;
      });

      /// add all new votes
      for( int i = 0; i <= STEEM_MAX_PROXY_RECURSION_DEPTH; ++i )
         delta[i] = -delta[i];
      _db.adjust_proxied_witness_votes( account, delta );
   } else { /// we are clearing the proxy which means we simply update the account
      _db.modify( account, [&]( account_object& a ) {
          a.proxy = o.proxy;
      });
   }
}


void account_witness_vote_evaluator::do_apply( const account_witness_vote_operation& o )
{
   const auto& voter = _db.get_account( o.account );
   FC_ASSERT( voter.proxy.size() == 0, "A proxy is currently set, please clear the proxy before voting for a witness." );

   if( o.approve )
      FC_ASSERT( true, "Account has declined its voting rights." );

   const auto& witness = _db.get_witness( o.witness );

   const auto& by_account_witness_idx = _db.get_index< witness_vote_index >().indices().get< by_account_witness >();
   auto itr = by_account_witness_idx.find( boost::make_tuple( voter.name, witness.owner ) );

   if( itr == by_account_witness_idx.end() ) {
      FC_ASSERT( o.approve, "Vote doesn't exist, user must indicate a desire to approve witness." );

      FC_ASSERT( voter.witnesses_voted_for < STEEM_MAX_ACCOUNT_WITNESS_VOTES, "Account has voted for too many witnesses." ); // TODO: Remove after hardfork 2

      _db.create<witness_vote_object>( [&]( witness_vote_object& v ) {
           v.witness = witness.owner;
           v.account = voter.name;
      });

      _db.adjust_witness_vote( witness, voter.witness_vote_weight() );

      _db.modify( voter, [&]( account_object& a ) {
         a.witnesses_voted_for++;
      });

   } else {
      FC_ASSERT( !o.approve, "Vote currently exists, user must indicate a desire to reject witness." );

      _db.adjust_witness_vote( witness, -voter.witness_vote_weight() );

      _db.modify( voter, [&]( account_object& a ) {
         a.witnesses_voted_for--;
      });
      _db.remove( *itr );
   }
}

void custom_evaluator::do_apply( const custom_operation& o ){}

void custom_json_evaluator::do_apply( const custom_json_operation& o )
{
   database& d = db();
   std::shared_ptr< custom_operation_interpreter > eval = d.get_custom_json_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      if( d.is_producing() )
         throw e;
   }
   catch(...)
   {
      elog( "Unexpected exception applying custom json evaluator." );
   }
}


void custom_binary_evaluator::do_apply( const custom_binary_operation& o )
{
   database& d = db();

   std::shared_ptr< custom_operation_interpreter > eval = d.get_custom_json_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      if( d.is_producing() )
         throw e;
   }
   catch(...)
   {
      elog( "Unexpected exception applying custom json evaluator." );
   }
}


void feed_publish_evaluator::do_apply( const feed_publish_operation& o )
{
   FC_ASSERT( is_asset_type( o.exchange_rate.base, SBD_SYMBOL ) && is_asset_type( o.exchange_rate.quote, STEEM_SYMBOL ),
            "Price feed must be a SBD/STEEM price" );

   const auto& witness = _db.get_witness( o.publisher );
   _db.modify( witness, [&]( witness_object& w )
   {
      w.sbd_exchange_rate = o.exchange_rate;
      w.last_sbd_exchange_update = _db.head_block_time();
   });
}

void report_over_production_evaluator::do_apply( const report_over_production_operation& o )
{
   FC_ASSERT( false, "report_over_production_operation is disabled." );
}

void placeholder_a_evaluator::do_apply( const placeholder_a_operation& o )
{
   FC_ASSERT( false, "This is not a valid op." );
}

void placeholder_b_evaluator::do_apply( const placeholder_b_operation& o )
{
   FC_ASSERT( false, "This is not a valid op" );
}

void request_account_recovery_evaluator::do_apply( const request_account_recovery_operation& o )
{
   const auto& account_to_recover = _db.get_account( o.account_to_recover );

   if ( account_to_recover.recovery_account.length() )   // Make sure recovery matches expected recovery account
      FC_ASSERT( account_to_recover.recovery_account == o.recovery_account, "Cannot recover an account that does not have you as there recovery partner." );
   else                                                  // Empty string recovery account defaults to top witness
      FC_ASSERT( _db.get_index< witness_index >().indices().get< by_vote_name >().begin()->owner == o.recovery_account, "Top witness must recover an account with no recovery partner." );

   const auto& recovery_request_idx = _db.get_index< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   if( request == recovery_request_idx.end() ) // New Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover using an impossible authority." );
      FC_ASSERT( o.new_owner_authority.weight_threshold, "Cannot recover using an open authority." );

      // Check accounts in the new authority exist
      for( auto& a : o.new_owner_authority.account_auths )
      {
         _db.get_account( a.first );
      }


      _db.create< account_recovery_request_object >( [&]( account_recovery_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.new_owner_authority = o.new_owner_authority;
         req.expires = _db.head_block_time() + STEEM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
   else if( o.new_owner_authority.weight_threshold == 0 ) // Cancel Request if authority is open
   {
      _db.remove( *request );
   }
   else // Change Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover using an impossible authority." );

      // Check accounts in the new authority exist

      for( auto& a : o.new_owner_authority.account_auths )
      {
         _db.get_account( a.first );
      }

      _db.modify( *request, [&]( account_recovery_request_object& req )
      {
         req.new_owner_authority = o.new_owner_authority;
         req.expires = _db.head_block_time() + STEEM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
}

void recover_account_evaluator::do_apply( const recover_account_operation& o )
{
   const auto& account = _db.get_account( o.account_to_recover );

   FC_ASSERT( _db.head_block_time() - account.last_account_recovery > STEEM_OWNER_UPDATE_LIMIT, "Owner authority can only be updated once an hour." );

   const auto& recovery_request_idx = _db.get_index< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   FC_ASSERT( request != recovery_request_idx.end(), "There are no active recovery requests for this account." );
   FC_ASSERT( request->new_owner_authority == o.new_owner_authority, "New owner authority does not match recovery request." );

   const auto& recent_auth_idx = _db.get_index< owner_authority_history_index >().indices().get< by_account >();
   auto hist = recent_auth_idx.lower_bound( o.account_to_recover );
   bool found = false;

   while( hist != recent_auth_idx.end() && hist->account == o.account_to_recover && !found )
   {
      found = hist->previous_owner_authority == o.recent_owner_authority;
      if( found ) break;
      ++hist;
   }

   FC_ASSERT( found, "Recent authority not found in authority history." );

   _db.remove( *request ); // Remove first, update_owner_authority may invalidate iterator
   _db.update_owner_authority( account, o.new_owner_authority );
   _db.modify( account, [&]( account_object& a )
   {
      a.last_account_recovery = _db.head_block_time();
   });
}

void change_recovery_account_evaluator::do_apply( const change_recovery_account_operation& o )
{
   _db.get_account( o.new_recovery_account ); // Simply validate account exists
   const auto& account_to_recover = _db.get_account( o.account_to_recover );

   const auto& change_recovery_idx = _db.get_index< change_recovery_account_request_index >().indices().get< by_account >();
   auto request = change_recovery_idx.find( o.account_to_recover );

   if( request == change_recovery_idx.end() ) // New request
   {
      _db.create< change_recovery_account_request_object >( [&]( change_recovery_account_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.recovery_account = o.new_recovery_account;
         req.effective_on = _db.head_block_time() + STEEM_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else if( account_to_recover.recovery_account != o.new_recovery_account ) // Change existing request
   {
      _db.modify( *request, [&]( change_recovery_account_request_object& req )
      {
         req.recovery_account = o.new_recovery_account;
         req.effective_on = _db.head_block_time() + STEEM_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else // Request exists and changing back to current recovery account
   {
      _db.remove( *request );
   }
}

void reset_account_evaluator::do_apply( const reset_account_operation& op )
{
   FC_ASSERT( false, "Reset Account Operation is currently disabled." );
/*
   const auto& acnt = _db.get_account( op.account_to_reset );
   auto band = _db.find< account_bandwidth_object, by_account_bandwidth_type >( boost::make_tuple( op.account_to_reset, bandwidth_type::old_forum ) );
   if( band != nullptr )
      FC_ASSERT( ( _db.head_block_time() - band->last_bandwidth_update ) > fc::days(60), "Account must be inactive for 60 days to be eligible for reset" );
   FC_ASSERT( acnt.reset_account == op.reset_account, "Reset account does not match reset account on account." );

   _db.update_owner_authority( acnt, op.new_owner_authority );
*/
}

void set_reset_account_evaluator::do_apply( const set_reset_account_operation& op )
{
   FC_ASSERT( false, "Set Reset Account Operation is currently disabled." );
/*
   const auto& acnt = _db.get_account( op.account );
   _db.get_account( op.reset_account );

   FC_ASSERT( acnt.reset_account == op.current_reset_account, "Current reset account does not match reset account on account." );
   FC_ASSERT( acnt.reset_account != op.reset_account, "Reset account must change" );

   _db.modify( acnt, [&]( account_object& a )
   {
       a.reset_account = op.reset_account;
   });
*/
}

void application_create_evaluator::do_apply( const application_create_operation& o )
{
   const auto& author = _db.get_account( o.author );

   verify_authority_accounts_exist( _db, o.active, o.author, authority::active );

   //TODO remove new_application
   const auto& new_application = _db.create< application_object >( [&]( application_object& app )
                                                           {
                                                                app.name = o.name;
                                                                app.author = o.author;
                                                                app.price_param = static_cast<application_object::application_price_param>(o.price_param);
                                                                app.url = o.url;
#ifndef IS_LOW_MEM
                                                                from_string( app.metadata, o.metadata );
#endif
                                                           });
}


void application_update_evaluator::do_apply( const application_update_operation& o )
{
   const auto& application = _db.get_application( o.name );

   verify_authority_accounts_exist( _db, o.active, o.author, authority::active );

   FC_ASSERT(application.author == o.author, "Provided author is not this applcation author" );

   _db.modify( application, [&]( application_object& app )
   {
      if(o.new_author)
        app.author = *o.new_author;

      if(o.price_param)
        app.price_param = static_cast<application_object::application_price_param>(*o.price_param);

#ifndef IS_LOW_MEM
        if ( o.metadata.size() > 0 )
           from_string( app.metadata, o.metadata );

        if ( o.url.size() > 0 )
           app.url = o.url;
#endif
   });


}

void application_delete_evaluator::do_apply( const application_delete_operation& o )
{
   const auto& application = _db.get_application( o.name );

   verify_authority_accounts_exist( _db, o.active, o.author, authority::active );

   FC_ASSERT(application.author == o.author, "Provided author is not this applcation author" );

   _db.remove(application);
}


#ifdef STEEM_ENABLE_SMT
void claim_reward_balance2_evaluator::do_apply( const claim_reward_balance2_operation& op )
{
   const account_object* a = nullptr; // Lazily initialized below because it may turn out unnecessary.

   for( const asset& token : op.reward_tokens )
   {
      if( token.amount == 0 )
         continue;
         
      if( token.symbol.space() == asset_symbol_type::smt_nai_space )
      {
         _db.adjust_reward_balance( op.account, -token );
         _db.adjust_balance( op.account, token );
      }
      else
      {
         // Lazy init here.
         if( a == nullptr )
         {
            a = _db.find_account( op.account );
            FC_ASSERT( a != nullptr, "Could NOT find account ${a}", ("a", op.account) );
         }

         if( token.symbol == VESTS_SYMBOL)
         {
            FC_ASSERT( token <= a->reward_vesting_balance, "Cannot claim that much VESTS. Claim: ${c} Actual: ${a}",
               ("c", token)("a", a->reward_vesting_balance) );   

            asset reward_vesting_steem_to_move = asset( 0, STEEM_SYMBOL );
            if( token == a->reward_vesting_balance )
               reward_vesting_steem_to_move = a->reward_vesting_steem;
            else
               reward_vesting_steem_to_move = asset( ( ( uint128_t( token.amount.value ) * uint128_t( a->reward_vesting_steem.amount.value ) )
                  / uint128_t( a->reward_vesting_balance.amount.value ) ).to_uint64(), STEEM_SYMBOL );

            _db.modify( *a, [&]( account_object& a )
            {
               a.vesting_shares += token;
               a.reward_vesting_balance -= token;
               a.reward_vesting_steem -= reward_vesting_steem_to_move;
            });

            _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
            {
               gpo.total_vesting_shares += token;
               gpo.total_vesting_fund_steem += reward_vesting_steem_to_move;

               gpo.pending_rewarded_vesting_shares -= token;
               gpo.pending_rewarded_vesting_steem -= reward_vesting_steem_to_move;
            });

            _db.adjust_proxied_witness_votes( *a, token.amount );
         }
         else if( token.symbol == STEEM_SYMBOL || token.symbol == SBD_SYMBOL )
         {
            FC_ASSERT( is_asset_type( token, STEEM_SYMBOL ) == false || token <= a->reward_steem_balance,
                       "Cannot claim that much STEEM. Claim: ${c} Actual: ${a}", ("c", token)("a", a->reward_steem_balance) );
            FC_ASSERT( is_asset_type( token, SBD_SYMBOL ) == false || token <= a->reward_sbd_balance,
                       "Cannot claim that much SBD. Claim: ${c} Actual: ${a}", ("c", token)("a", a->reward_sbd_balance) );
            _db.adjust_reward_balance( *a, -token );
            _db.adjust_balance( *a, token );
         }
         else
            FC_ASSERT( false, "Unknown asset symbol" );
      } // non-SMT token
   } // for( const auto& token : op.reward_tokens )
}
#endif


} } // steem::chain
