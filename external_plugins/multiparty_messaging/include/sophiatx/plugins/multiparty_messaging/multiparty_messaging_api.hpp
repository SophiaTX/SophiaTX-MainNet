#pragma once
#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_objects.hpp>
#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_plugin.hpp>

#include <sophiatx/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>

namespace sophiatx { namespace plugins { namespace multiparty_messaging {

namespace detail
{
   class multiparty_messaging_api_impl;
}

class api_group_object {
public:
   api_group_object(){}
   api_group_object(const group_object& go):group_name(go.group_name),current_group_name(go.current_group_name),admin(go.admin), group_key(go.group_key), messages(go.current_seq)
   {
      description = to_string(go.description);
      std::copy(go.members.begin(), go.members.end(), std::back_inserter(members));
   }

   account_name_type group_name;
   account_name_type current_group_name;

   string description;

   std::vector < account_name_type > members;
   account_name_type admin;

   fc::sha256 group_key;
   uint32_t   messages = 0;
};

class api_message_object {
public:
   api_message_object(){}
   api_message_object(const message_object& mo):group_name(mo.group_name), sequence(mo.sequence), sender(mo.sender), system_message(mo.system_message)
   {
      std::copy(mo.recipients.begin(), mo.recipients.end(), std::back_inserter(recipients));
      std::copy(mo.data.begin(), mo.data.end(), std::back_inserter(data));
   }

   account_name_type group_name;
   uint32_t          sequence;
   account_name_type sender;
   std::vector < account_name_type > recipients;
   std::vector<char>      data;
   bool system_message = false;
};

struct get_group_args{
   account_name_type group_name;
};

typedef optional<api_group_object> get_group_return;

struct get_group_name_args{
   account_name_type current_group_name;
};

typedef account_name_type get_group_name_return;

struct list_my_groups_args{
   string   start;
   uint32_t count;
};

typedef vector<api_group_object> list_my_groups_return;

struct list_messages_args{
   account_name_type group_name;
   uint32_t start;
   uint32_t count;
};

typedef vector<api_message_object> list_messages_return;

struct create_group_args{
   std::vector<account_name_type> members;
   string description;
   account_name_type admin;
};

struct create_group_return{
   account_name_type group_name;
   std::map<account_name_type, group_meta> operation_payloads;
};

struct add_group_participants_args{
   account_name_type group_name;
   std::vector<account_name_type> new_members;
   account_name_type admin;
   bool check_members;
};

typedef std::map<account_name_type, group_meta> add_group_participants_return;

struct delete_group_participants_args{
   account_name_type group_name;
   std::vector<account_name_type> deleted_members;
   account_name_type admin;
   bool check_members;
};

typedef std::map<account_name_type, group_meta> delete_group_participants_return;

struct update_group_args{
   account_name_type group_name;
   string description;
   account_name_type admin;
};

typedef std::map<account_name_type, group_meta> update_group_return;

struct disband_group_args{
   account_name_type group_name;
   account_name_type admin;
};

typedef std::map<account_name_type, group_meta> disband_group_return;

struct send_group_message_args{
   account_name_type group_name;
   account_name_type sender;
   std::vector<char> data;
};

typedef std::map<account_name_type, group_meta> send_group_message_return;


class multiparty_messaging_api
{
   public:
      multiparty_messaging_api(multiparty_messaging_plugin& plugin);
      ~multiparty_messaging_api();
      void api_startup();

      DECLARE_API( (get_group) (get_group_name) (list_my_groups) (list_messages)
                   (create_group) (add_group_participants) (delete_group_participants) (update_group) (disband_group) (send_group_message))

   private:
      std::unique_ptr< detail::multiparty_messaging_api_impl > my;
};

} } } // sophiatx::plugins::multiparty_messaging


FC_REFLECT( sophiatx::plugins::multiparty_messaging::api_group_object, (group_name)(current_group_name)(description)(members)(admin)(group_key)(messages) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::api_message_object, (group_name)(sender)(recipients)(data)(system_message)(sequence) )

FC_REFLECT( sophiatx::plugins::multiparty_messaging::get_group_args,
            (group_name) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::get_group_name_args,
            (current_group_name) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::list_my_groups_args,
            (start)(count) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::list_messages_args,
            (group_name)(start)(count) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::create_group_args,
            (members)(description)(admin) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::create_group_return,
            (group_name)(operation_payloads) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::add_group_participants_args,
            (group_name)(new_members)(admin)(check_members) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::delete_group_participants_args,
            (group_name)(deleted_members)(admin)(check_members) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::update_group_args,
            (group_name)(description)(admin) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::disband_group_args,
            (group_name)(admin) )
FC_REFLECT( sophiatx::plugins::multiparty_messaging::send_group_message_args,
            (group_name)(sender)(data) )
