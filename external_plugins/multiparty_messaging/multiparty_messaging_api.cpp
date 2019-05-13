#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_plugin.hpp>
#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_api.hpp>
#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_objects.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api.hpp>

#include <fc/crypto/aes.hpp>

namespace sophiatx { namespace plugins { namespace multiparty_messaging {

namespace detail {


vector<char> encode_message( const message_wrapper& message, const fc::sha256& iv, const fc::sha256& key )
{
   vector<char> raw_msg = fc::raw::pack_to_vector( message );
   return fc::aes_encrypt( key, iv, raw_msg);
}

vector<char> encode_message( const message_wrapper& message, const fc::sha512& key )
{
   vector<char> raw_msg = fc::raw::pack_to_vector( message );
   return fc::aes_encrypt( key, raw_msg);
}

void find_new_members ( shared_vector<account_name_type> current_members, vector<account_name_type> adding, vector<account_name_type>& new_members, vector<account_name_type>& all_members)
{
   std::sort(current_members.begin(), current_members.end());
   std::sort(adding.begin(), adding.end());
   all_members.clear();
   new_members.clear();
   std::merge(current_members.begin(), current_members.end(), adding.begin(), adding.end(), std::back_inserter(all_members));
   std::set_difference(all_members.begin(), all_members.end(), current_members.begin(), current_members.end(), std::back_inserter(new_members));
}


class multiparty_messaging_api_impl
{
   public:
   multiparty_messaging_api_impl(multiparty_messaging_plugin& plugin) : _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ), _plugin(plugin) {}

    DECLARE_API_IMPL((get_group) (get_group_name) (list_my_groups) (list_messages) (create_group) (add_group_participants) (delete_group_participants) (update_group) (disband_group) (send_group_message))

   std::shared_ptr<database_interface> _db;
   multiparty_messaging_plugin& _plugin;
   plugins::json_rpc::json_rpc_plugin* json_api;

private:
   vector<char> generate_random_key() const;
   alexandria_api::api_account_object get_account(const account_name_type& account) const;
   string suggest_group_name( const string& description)const;
   group_meta encode_and_pack(const fc::sha256& key, const fc::sha256& iv, const group_op& op ) const;
   group_meta encode_and_pack(const fc::sha512& shared_secret, const group_op& op )const;
};

group_meta multiparty_messaging_api_impl::encode_and_pack(const fc::sha256& key, const fc::sha256& iv, const group_op& op )const
{
   message_wrapper message_content;
   message_content.type = message_wrapper::message_type::group_operation;
   message_content.operation_data = op;

   vector<char> encoded_message = encode_message(message_content, iv, key);
   group_meta message_meta;
   message_meta.data.clear();
   message_meta.data = encoded_message;
   message_meta.iv = iv;
   return message_meta;
}

group_meta multiparty_messaging_api_impl::encode_and_pack(const fc::sha512& shared_secret, const group_op& op )const
{
   message_wrapper message_content;
   message_content.type = message_wrapper::message_type::group_operation;
   message_content.operation_data = op;

   vector<char> encoded_message = encode_message(message_content, shared_secret);
   group_meta message_meta;
   message_meta.data.clear();
   message_meta.data = encoded_message;
   return message_meta;
}

string multiparty_messaging_api_impl::suggest_group_name( const string& description)const
{
   string seed = description + fc::ecc::private_key::generate().get_secret().str();
   auto hash = fc::ripemd160::hash(seed);
   unsigned char data[21];
   memcpy(data, hash.data(), 20);
   data[20] = 0; //do the padding to avoid '=' at the end of the result string
   return fc::base64m_encode(data, 21);
}

vector<char> multiparty_messaging_api_impl::generate_random_key() const
{
   fc::sha256::encoder sha_enc;
   for(int i=0; i<4; i++) {
      fc::sha256 sha_entropy = fc::ecc::private_key::generate().get_secret();
      sha_enc.write( sha_entropy.data(), sha_entropy.data_size());
   }
   fc::sha256 result_key = sha_enc.result();
   vector<char> ret(result_key.data(), result_key.data() + result_key.data_size());
   return ret;
}

alexandria_api::api_account_object multiparty_messaging_api_impl::get_account(const account_name_type& account) const {
   alexandria_api::get_account_args args {account};

   auto result = json_api->call_api_method("alexandria_api", "get_account", fc::variant(args), [](fc::variant& v, uint64_t i){ FC_UNUSED(v) FC_UNUSED(i)} );

   FC_ASSERT(result.valid(), "Account does not exist!");
   alexandria_api::get_account_return acc_return;
   fc::from_variant( *result, acc_return );
   FC_ASSERT(acc_return.account.size(), "Account does not exist!");
   return acc_return.account[0];
}

DEFINE_API_IMPL( multiparty_messaging_api_impl, get_group)
{
   get_group_return final_result;
   const auto& group_idx = _db->get_index< group_index >().indices().get< by_group_name >();
   const auto& group_itr = group_idx.find(args.group_name);
   if(group_itr != group_idx.end())
      final_result = *group_itr;
   return final_result;
}

DEFINE_API_IMPL( multiparty_messaging_api_impl, get_group_name)
{
   get_group_name_return final_result;
   const auto& group_idx = _db->get_index< group_index >().indices().get< by_current_name >();
   const auto& group_itr = group_idx.find(args.current_group_name);
   if(group_itr != group_idx.end())
      final_result = group_itr->group_name;
   else
      final_result = "";
   return final_result;
}

DEFINE_API_IMPL( multiparty_messaging_api_impl, list_my_groups)
{
   list_my_groups_return ret;
   FC_ASSERT(args.count<1000);
   const auto& group_idx = _db->get_index< group_index >().indices().get< by_group_name >();
   auto group_itr = group_idx.lower_bound(args.start);
   while( group_itr != group_idx.end() && ret.size() < args.count) {
      ret.push_back(*group_itr);
      group_itr++;
   }
   return ret;
}

DEFINE_API_IMPL( multiparty_messaging_api_impl, list_messages)
{
   list_messages_return ret;
   FC_ASSERT(args.count<1000);
   const auto& message_idx = _db->get_index< message_index >().indices().get< by_group_seq >();
   auto message_itr = message_idx.find( std::make_tuple(args.group_name, args.start));
   while( message_itr != message_idx.end() && message_itr->group_name == args.group_name && ret.size() < args.count) {
      ret.push_back(*message_itr);
      message_itr++;
   }
   return ret;
}


DEFINE_API_IMPL( multiparty_messaging_api_impl, create_group)
{
   create_group_return ret;
   auto admin = get_account(args.admin);

   account_name_type group_name = suggest_group_name(args.description);

   vector<char> group_key = generate_random_key();
   auto pk_itr = _plugin._private_keys.find(admin.memo_key);
   FC_ASSERT( pk_itr != _plugin._private_keys.end(), "the respective key not imported" );

   vector<account_name_type> members;
   std::copy( args.members.begin(), args.members.end(), std::back_inserter(members) );
   std::sort(members.begin(), members.end());
   auto last = std::unique(members.begin(), members.end());
   members.erase(last, members.end());

   for( auto m: members ){
      auto member = get_account(m);
      group_op g_op( "add", group_name, args.description, members, admin.memo_key);
      g_op.user_list->push_back(admin.name);
      fc::sha512 sc = pk_itr->second.get_shared_secret(member.memo_key);
      vector<char> encrypted_key = fc::aes_encrypt( sc, group_key );
      g_op.new_key[member.memo_key] = encrypted_key;

      group_meta message_meta = encode_and_pack(sc, g_op);
      message_meta.sender = admin.memo_key;
      message_meta.recipient = member.memo_key;
      ret.operation_payloads[member.name] = message_meta;
   }
   ret.group_name = group_name;
   return ret;
}

DEFINE_API_IMPL( multiparty_messaging_api_impl, add_group_participants)
{
   add_group_participants_return ret;
   const group_object* g_ob = _db->find< group_object, by_current_name >( args.group_name );
   if(!g_ob)  g_ob = _db->find< group_object, by_group_name >( args.group_name );
   FC_ASSERT( g_ob, "Group not found");
   FC_ASSERT( g_ob->admin == args.admin, "you are not group admin" );
   auto admin = get_account(args.admin);

   account_name_type new_group_name = suggest_group_name(to_string(g_ob->description));  //add some more entrophy in this
   vector<char> new_group_key = generate_random_key();
   auto pk_itr = _plugin._private_keys.find(admin.memo_key);
   FC_ASSERT( pk_itr != _plugin._private_keys.end(), "the respective key not imported" );

   vector<account_name_type> all_members;
   vector<account_name_type> new_members;

   if(args.check_members) {
      find_new_members(g_ob->members, args.new_members, new_members, all_members);
   }else {
      std::copy(g_ob->members.begin(), g_ob->members.end(), std::back_inserter(all_members));
      std::copy(args.new_members.begin(), args.new_members.end(), std::back_inserter(all_members));
      std::copy(args.new_members.begin(), args.new_members.end(), std::back_inserter(new_members));
   }

   for( auto m: new_members ){
      auto member = get_account(m);
      group_op g_op( "add", new_group_name, to_string(g_ob->description), all_members, admin.memo_key);

      fc::sha512 sc = pk_itr->second.get_shared_secret(member.memo_key);
      vector<char> encrypted_key = fc::aes_encrypt( sc, new_group_key );
      g_op.new_key[member.memo_key] = encrypted_key;

      group_meta message_meta = encode_and_pack(sc, g_op);
      message_meta.sender = admin.memo_key;
      message_meta.recipient = member.memo_key;
      ret[member.name] = message_meta;
   }
   {
      vector<char> iv_v = generate_random_key();
      fc::sha256 iv( iv_v.data(), iv_v.size());
      group_op g_op( "update", new_group_name, to_string(g_ob->description), all_members, admin.memo_key);
      vector<char> encrypted_key = fc::aes_encrypt( g_ob->group_key, iv, new_group_key );
      g_op.new_key[public_key_type()] = encrypted_key;

      group_meta message_meta = encode_and_pack(g_ob->group_key, iv, g_op);
      ret[g_ob->current_group_name] = message_meta;
   }

   return ret;
}

DEFINE_API_IMPL( multiparty_messaging_api_impl, delete_group_participants)
{
   delete_group_participants_return ret;
   const group_object* g_ob = _db->find< group_object, by_current_name >( args.group_name );
   if(!g_ob)  g_ob = _db->find< group_object, by_group_name >( args.group_name );
   FC_ASSERT( g_ob, "Group not found");
   FC_ASSERT( g_ob->admin == args.admin, "you are not group admin" );
   auto admin = get_account(args.admin);

   account_name_type new_group_name = suggest_group_name(to_string(g_ob->description));
   vector<char> new_group_key = generate_random_key();
   auto pk_itr = _plugin._private_keys.find(admin.memo_key);
   FC_ASSERT( pk_itr != _plugin._private_keys.end(), "the respective key not imported" );

   vector<account_name_type> all_members;
   vector<account_name_type> to_remove;
   vector<account_name_type> new_members;

   std::copy(g_ob->members.begin(), g_ob->members.end(), std::back_inserter(all_members));
   std::sort(all_members.begin(), all_members.end());
   auto last1 = std::unique(all_members.begin(), all_members.end());
   all_members.erase(last1, all_members.end());

   std::copy(args.deleted_members.begin(), args.deleted_members.end(), std::back_inserter(to_remove));
   std::sort(to_remove.begin(), to_remove.end());
   auto last2 = std::unique(to_remove.begin(), to_remove.end());
   to_remove.erase(last2, to_remove.end());

   std::set_difference (all_members.begin(), all_members.end(), to_remove.begin(), to_remove.end(), std::back_inserter(new_members));

   {
      vector<char> iv_v = generate_random_key();
      fc::sha256 iv( iv_v.data(), iv_v.size());
      group_op g_op( "update", new_group_name, to_string(g_ob->description), all_members, admin.memo_key);
      //TODO - we should send two updates instead of one, so old members won't learn the new group name
      for( auto m: new_members ){
         auto member = get_account(m);
         fc::sha512 sc = pk_itr->second.get_shared_secret(member.memo_key);
         vector<char> encrypted_key = fc::aes_encrypt( sc, new_group_key );
         g_op.new_key[member.memo_key] = encrypted_key;
      }
      ret[g_ob->current_group_name] = encode_and_pack(g_ob->group_key, iv, g_op);
   }

   return ret;
}


DEFINE_API_IMPL( multiparty_messaging_api_impl, update_group)
{
   update_group_return ret;
   const group_object* g_ob = _db->find< group_object, by_current_name >( args.group_name );
   if(!g_ob)  g_ob = _db->find< group_object, by_group_name >( args.group_name );
   FC_ASSERT( g_ob, "Group not found");
   FC_ASSERT( g_ob->admin == args.admin, "you are not group admin" );
   auto admin = get_account(args.admin);

   account_name_type new_group_name = suggest_group_name(to_string(g_ob->description));  //add some more entrophy in this
   vector<char> new_group_key = generate_random_key();
   auto pk_itr = _plugin._private_keys.find(admin.memo_key);
   FC_ASSERT( pk_itr != _plugin._private_keys.end(), "the respective key not imported" );

   vector<account_name_type> all_members;
   std::copy(g_ob->members.begin(), g_ob->members.end(), std::back_inserter(all_members));

   {
      vector<char> iv_v = generate_random_key();
      fc::sha256 iv( iv_v.data(), iv_v.size());
      group_op g_op( "update", new_group_name, args.description, all_members, admin.memo_key);
      vector<char> encrypted_key = fc::aes_encrypt( g_ob->group_key, iv, new_group_key );
      g_op.new_key[public_key_type()] = encrypted_key;

      ret[g_ob->current_group_name] = encode_and_pack(g_ob->group_key, iv, g_op);
   }
   return ret;
}

DEFINE_API_IMPL( multiparty_messaging_api_impl, disband_group)
{
   disband_group_return ret;
   const group_object* g_ob = _db->find< group_object, by_current_name >( args.group_name );
   if(!g_ob)  g_ob = _db->find< group_object, by_group_name >( args.group_name );
   FC_ASSERT( g_ob, "Group not found");
   FC_ASSERT( g_ob->admin == args.admin, "you are not group admin" );
   auto admin = get_account(args.admin);

   auto pk_itr = _plugin._private_keys.find(admin.memo_key);
   FC_ASSERT( pk_itr != _plugin._private_keys.end(), "the respective key not imported" );

   {
      vector<char> iv_v = generate_random_key();
      fc::sha256 iv( iv_v.data(), iv_v.size());
      group_op g_op( "disband", g_ob->group_name, "", {}, admin.memo_key);

      ret[g_ob->current_group_name] = encode_and_pack(g_ob->group_key, iv, g_op);
   }
   return ret;
}

DEFINE_API_IMPL( multiparty_messaging_api_impl, send_group_message)
{
   send_group_message_return ret;
   const group_object* g_ob = _db->find< group_object, by_current_name >( args.group_name );
   if(!g_ob)  g_ob = _db->find< group_object, by_group_name >( args.group_name );
   FC_ASSERT( g_ob, "Group not found");
   auto sender = get_account(args.sender);

   auto pk_itr = _plugin._private_keys.find(sender.memo_key);
   FC_ASSERT( pk_itr != _plugin._private_keys.end(), "the respective key not imported" );
   {
      vector<char> iv_v = generate_random_key();
      fc::sha256 iv( iv_v.data(), iv_v.size());

      message_wrapper message_content;
      message_content.type = message_wrapper::message_type::message;
      message_content.message_data = args.data;

      vector<char> encoded_message = encode_message(message_content, iv, g_ob->group_key);
      group_meta message_meta;
      message_meta.data.clear();
      message_meta.data = encoded_message;
      message_meta.iv = iv;
      ret[g_ob->current_group_name] = message_meta;
   }
   return ret;
}

} // detail

multiparty_messaging_api::multiparty_messaging_api(multiparty_messaging_plugin& plugin): my( new detail::multiparty_messaging_api_impl(plugin) )
{
   JSON_RPC_REGISTER_API( SOPHIATX_MPM_PLUGIN_NAME );
}

multiparty_messaging_api::~multiparty_messaging_api() {}

void multiparty_messaging_api::api_startup() {
   my->json_api = appbase::app().find_plugin< plugins::json_rpc::json_rpc_plugin >();
}

DEFINE_READ_APIS( multiparty_messaging_api, (get_group) (get_group_name) (list_my_groups) (list_messages)
                   (create_group) (add_group_participants) (delete_group_participants) (update_group) (disband_group) (send_group_message))

} } } // sophiatx::plugins::multiparty_messaging
