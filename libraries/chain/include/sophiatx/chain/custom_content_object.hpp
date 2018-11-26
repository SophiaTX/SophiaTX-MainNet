#pragma once

#include <sophiatx/protocol/authority.hpp>
#include <sophiatx/protocol/operations.hpp>
#include <sophiatx/protocol/sophiatx_operations.hpp>

#include <sophiatx/chain/buffer_type.hpp>
#include <sophiatx/chain/sophiatx_object_types.hpp>
#include <sophiatx/chain/witness_objects.hpp>
#include <fc/time.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <sophiatx/protocol/config.hpp>



namespace sophiatx { namespace chain {

class custom_content_object: public object< custom_content_object_type, custom_content_object> {
public:
   template<typename Constructor, typename Allocator>
   custom_content_object(Constructor &&c, allocator<Allocator> a):all_recipients( a.get_segment_manager() ), data(a), json(a) {
      c(*this);
   }

   id_type id;

   uint64_t app_id;
   account_name_type sender;
   account_name_type recipient;

   shared_vector<account_name_type> all_recipients;

   uint64_t sender_sequence = 0;
   uint64_t recipient_sequence = 0;
   uint64_t app_message_sequence = 0;
   time_point_sec received;

   bool binary;
   shared_vector<char> data;
   shared_string json;
};

struct by_id;
struct by_app_id;
struct by_sender;
struct by_recipient;
struct by_sender_time;
struct by_recipient_time;

typedef multi_index_container<
      custom_content_object,
      indexed_by<
            ordered_unique< tag< by_id >,
                    member< custom_content_object, custom_content_object::id_type, &custom_content_object::id > >,
            ordered_non_unique< tag< by_app_id >,
                composite_key< custom_content_object,
                    member< custom_content_object, uint64_t, &custom_content_object::app_id>,
                    member< custom_content_object, uint64_t, &custom_content_object::app_message_sequence>
                >,
                composite_key_compare< std::greater<uint64_t>, std::greater<uint64_t> >
            >,
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


FC_REFLECT(sophiatx::chain::custom_content_object,
           (id)(app_id)(sender)(recipient)(binary)(data)(json)(received)(sender_sequence)(recipient_sequence)(app_message_sequence)
)
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::custom_content_object, sophiatx::chain::custom_content_index )
