#pragma once

#include <steem/protocol/authority.hpp>
#include <steem/protocol/operations.hpp>
#include <steem/protocol/steem_operations.hpp>

#include <steem/chain/buffer_type.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/chain/witness_objects.hpp>
#include <fc/time.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <steem/protocol/config.hpp>



namespace steem { namespace chain {

class custom_content_object: public object< custom_content_object_type, custom_content_object> {
public:
   template<typename Constructor, typename Allocator>
   custom_content_object(Constructor &&c, allocator<Allocator> a) {
      c(*this);
   }

   custom_content_object() {}

   id_type id;

   uint64_t app_id;
   account_name_type sender;
   account_name_type recipient;
   flat_set<account_name_type> all_recipients;

   uint64_t sender_sequence = 0;
   uint64_t recipient_sequence = 0;
   time_point_sec received;

   bool binary;
   vector<char> data;
   string json;
};

struct by_id;
struct by_sender;
struct by_recipient;
struct by_sender_time;
struct by_recipient_time;

typedef multi_index_container<
      custom_content_object,
      indexed_by<
            ordered_unique< tag< by_id >,
                  member< custom_content_object, custom_content_object::id_type, &custom_content_object::id > >,
            ordered_non_unique< tag< by_sender >,
               composite_key< custom_content_object,
                     member< custom_content_object, account_name_type, &custom_content_object::sender>,
                     member< custom_content_object, uint64_t, &custom_content_object::app_id>,
                     member< custom_content_object, uint64_t, &custom_content_object::sender_sequence>
               >,
               composite_key_compare< std::less< account_name_type >, std::greater<uint64_t>, std::greater< uint64_t > >
            >,
            ordered_non_unique< tag< by_recipient >,
               composite_key< custom_content_object,
                     member< custom_content_object, account_name_type, &custom_content_object::recipient>,
                     member< custom_content_object, uint64_t, &custom_content_object::app_id>,
                     member< custom_content_object, uint64_t, &custom_content_object::recipient_sequence>
               >,
               composite_key_compare< std::less< account_name_type >, std::greater<uint64_t>, std::greater< uint64_t > >
            >,
            ordered_non_unique< tag< by_sender_time >,
               composite_key< custom_content_object,
                     member< custom_content_object, account_name_type, &custom_content_object::sender>,
                     member< custom_content_object, uint64_t, &custom_content_object::app_id>,
                     member< custom_content_object, time_point_sec, &custom_content_object::received>
               >,
               composite_key_compare< std::less< account_name_type >, std::greater<uint64_t>, std::greater< time_point_sec > >
            >,
            ordered_non_unique< tag< by_recipient_time >,
               composite_key< custom_content_object,
                     member< custom_content_object, account_name_type, &custom_content_object::recipient>,
                     member< custom_content_object, uint64_t, &custom_content_object::app_id>,
                     member< custom_content_object, time_point_sec, &custom_content_object::received>
               >,
               composite_key_compare< std::less< account_name_type >, std::greater<uint64_t>, std::greater< time_point_sec > >
            >
      >,
      allocator< custom_content_object >
> custom_content_index;


}} //namespace


FC_REFLECT(steem::chain::custom_content_object,
           (id)(app_id)(sender)(recipient)(binary)(data)(json)(received)(sender_sequence)(recipient_sequence)
)
CHAINBASE_SET_INDEX_TYPE( steem::chain::custom_content_object, steem::chain::custom_content_index )
