#pragma once
#include <sophiatx/chain/sophiatx_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace sophiatx { namespace plugins { namespace multiparty_messaging {

using namespace std;
using namespace sophiatx::chain;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various multiparty_messaging automagic depends on them being known at compile
// time.
//
#ifndef SOPHIATX_MULTIPARTY_MESSAGING_SPACE_ID
#define SOPHIATX_MULTIPARTY_MESSAGING_SPACE_ID 121
#endif

typedef vector<char> encrypted_key;

struct group_op{
   group_op(){}
   group_op( string _type, account_name_type _name, string _description, vector<account_name_type> _user_list, public_key_type _senders_pubkey ):
            type(_type), description (_description), senders_pubkey(_senders_pubkey)
   {
      if(_user_list.size()){
         vector<account_name_type> temp;
         std::copy(_user_list.begin(), _user_list.end(), std::back_inserter(temp));
         user_list = std::move(temp);
      }
      if(_name.size())
         new_group_name = _name;
   }

   uint32_t                   version = 1;
   string                     type; //"add" "disband" "update"
   string                     description;
   optional<account_name_type> new_group_name;
   optional<vector<account_name_type>>   user_list;
   public_key_type            senders_pubkey;
   std::map<public_key_type, encrypted_key> new_key;
};

struct message_wrapper{
   enum message_type{
      group_operation = 0,
      message = 1
   };
   uint32_t type = message_type::group_operation ;
   optional<vector<char>> message_data;
   optional<group_op>  operation_data;
};

struct group_meta{
   optional<public_key_type> sender;
   optional<public_key_type> recipient;
   optional<fc::sha256>      iv;
   vector<char>              data;
};

enum mpm_object_types
{
   group_object_type = ( SOPHIATX_MULTIPARTY_MESSAGING_SPACE_ID << 8 ),
   message_object_type = ( SOPHIATX_MULTIPARTY_MESSAGING_SPACE_ID << 8 )+1
};

class group_object : public object< group_object_type, group_object >
{
public:
   template< typename Constructor, typename Allocator >
   group_object( Constructor&& c, allocator< Allocator > a ): description(a), members(a.get_segment_manager())
   {
      c( *this );
   }
   id_type           id;

   account_name_type group_name;
   account_name_type current_group_name;

   shared_string description;

   shared_vector < account_name_type > members;
   account_name_type admin;

   fc::sha256 group_key;
   uint32_t   current_seq = 0;
};

class message_object : public object< message_object_type, message_object >
{
public:
   template< typename Constructor, typename Allocator >
   message_object( Constructor&& c, allocator< Allocator > a ): recipients(a.get_segment_manager()), data(a)
   {
      c( *this );
   }
   id_type           id;

   account_name_type group_name;
   uint32_t          sequence;
   account_name_type sender;
   shared_vector < account_name_type > recipients;
   shared_vector<char> data;
   bool system_message = false;
};


typedef group_object::id_type group_object_id_type;
typedef message_object::id_type message_object_id_type;


// group_operation
//
//

using namespace boost::multi_index;

struct by_group_seq;
struct by_group_name;
struct by_current_name;

typedef multi_index_container<
      message_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< message_object, message_object_id_type, &message_object::id > >,
      ordered_unique< tag <by_group_seq>,
            composite_key< message_object,
               member < message_object, account_name_type, &message_object::group_name >,
               member < message_object, uint32_t, &message_object::sequence >
            >,
            composite_key_compare< std::less< account_name_type >, std::less<uint64_t> >
      >
   >,
   allocator< message_object >
> message_index;

typedef multi_index_container<
      group_object,
      indexed_by<
            ordered_unique< tag< by_id >,        member< group_object, group_object_id_type, &group_object::id > >,
            ordered_unique< tag <by_group_name>, member< group_object, account_name_type, &group_object::group_name > >,
            ordered_unique< tag <by_current_name>, member< group_object, account_name_type, &group_object::current_group_name > >
      >,
      allocator< group_object >
> group_index;


} } } // sophiatx::plugins::multiparty_messaging


FC_REFLECT( sophiatx::plugins::multiparty_messaging::group_op, (version)(type)(new_group_name)(description)(user_list)(senders_pubkey)(new_key) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::group_meta, (sender)(recipient)(iv)(data))
FC_REFLECT( sophiatx::plugins::multiparty_messaging::group_object, (id)(group_name)(current_group_name)(description)(members)(admin)(group_key)(current_seq) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::message_object, (id)(group_name)(sender)(recipients)(data)(system_message)(sequence) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::message_wrapper, (type)(message_data)(operation_data) )

CHAINBASE_SET_INDEX_TYPE( sophiatx::plugins::multiparty_messaging::message_object, sophiatx::plugins::multiparty_messaging::message_index )
CHAINBASE_SET_INDEX_TYPE( sophiatx::plugins::multiparty_messaging::group_object, sophiatx::plugins::multiparty_messaging::group_index )
