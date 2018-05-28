#pragma once

#include <steem/protocol/authority.hpp>
#include <steem/protocol/operations.hpp>
#include <steem/protocol/steem_operations.hpp>

#include <steem/chain/buffer_type.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/chain/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>



namespace steem { namespace chain {

class custom_content_object: public object< custom_content_object_type, custom_content_object> {
public:
   template<typename Constructor, typename Allocator>
   custom_content_object(Constructor &&c, allocator<Allocator> a) {
      c(*this);
   }

   custom_content_object() {}

   id_type id;

   account_name_type sender;
   account_name_type recipient;
   flat_set<account_name_type> all_recipients;

   bool binary;
   vector<char> data;
   string json;
};

class by_id;
class by_sender;
class by_recipient;

typedef multi_index_container<
      custom_content_object,
      indexed_by<
            ordered_unique< tag< by_id >,
                  member< custom_content_object, custom_content_object::id_type, &custom_content_object::id > >,
            ordered_unique< tag< by_sender >,
                  member< custom_content_object, account_name_type, &custom_content_object::sender > >,
            ordered_unique< tag< by_recipient >,
                  member< custom_content_object, account_name_type, &custom_content_object::recipient > >
      >,
      allocator< custom_content_object >
> custom_content_index;


}} //namespace


FC_REFLECT(steem::chain::custom_content_object,
           (id)(sender)(recipient)(binary)(data)(json)
)
CHAINBASE_SET_INDEX_TYPE( steem::chain::custom_content_object, steem::chain::custom_content_index )
