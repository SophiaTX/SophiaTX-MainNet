#pragma once
#include <steem/protocol/sign_state.hpp>
#include <steem/protocol/exceptions.hpp>

namespace steem { namespace protocol {

template< typename AuthContainerType >
void verify_authority( const vector<AuthContainerType>& auth_containers, const flat_set<public_key_type>& sigs,
                       const authority_getter& get_active,
                       const authority_getter& get_owner,
                       uint32_t max_recursion_depth = STEEM_MAX_SIG_CHECK_DEPTH,
                       bool allow_committe = false,
                       const flat_set< account_name_type >& active_approvals = flat_set< account_name_type >(),
                       const flat_set< account_name_type >& owner_approvals = flat_set< account_name_type >()
                       )
{ try {
   flat_set< account_name_type > required_active;
   flat_set< account_name_type > required_owner;
   vector< authority > other;

   get_required_auth_visitor auth_visitor( required_active, required_owner, other );

   for( const auto& a : auth_containers )
      auth_visitor( a );


   flat_set< public_key_type > avail;
   sign_state s(sigs,get_active,avail);
   s.max_recursion = max_recursion_depth;
   for( auto& id : active_approvals )
      s.approved_by.insert( id );
   for( auto& id : owner_approvals )
      s.approved_by.insert( id );

   for( const auto& auth : other )
   {
      STEEM_ASSERT( s.check_authority(auth), tx_missing_other_auth, "Missing Authority", ("auth",auth)("sigs",sigs) );
   }

   // fetch all of the top level authorities
   for( const auto& id : required_active )
   {
      STEEM_ASSERT( s.check_authority(id) ||
                       s.check_authority(get_owner(id)),
                       tx_missing_active_auth, "Missing Active Authority ${id}", ("id",id)("auth",get_active(id))("owner",get_owner(id)) );
   }

   for( const auto& id : required_owner )
   {
      STEEM_ASSERT( owner_approvals.find(id) != owner_approvals.end() ||
                       s.check_authority(get_owner(id)),
                       tx_missing_owner_auth, "Missing Owner Authority ${id}", ("id",id)("auth",get_owner(id)) );
   }

   STEEM_ASSERT(
      !s.remove_unused_signatures(),
      tx_irrelevant_sig,
      "Unnecessary signature(s) detected"
      );
} FC_CAPTURE_AND_RETHROW( (auth_containers)(sigs) ) }

} } // steem::protocol
