#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_plugin.hpp>
#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_objects.hpp>
#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_api.hpp>

#include <sophiatx/chain/database.hpp>
#include <sophiatx/chain/index.hpp>
#include <sophiatx/chain/custom_operation_interpreter.hpp>
#include <sophiatx/chain/operation_notification.hpp>
#include <sophiatx/utilities/key_conversion.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/io/raw.hpp>

namespace sophiatx { namespace plugins { namespace multiparty_messaging_plugin {

namespace detail {

class multiparty_messaging_plugin_impl : public custom_operation_interpreter
{
public:
   multiparty_messaging_plugin_impl( multiparty_messaging_plugin& _plugin ) :
         _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ),
         app_id(_plugin.app_id) { }

   virtual ~multiparty_messaging_plugin_impl(){}
   database&                     _db;
   multiparty_messaging_plugin&  _self;
   std::map< sophiatx::protocol::public_key_type, fc::ecc::private_key > _private_keys;
   std::set< sophiatx::protocol::account_name_type >                     _accounts;
   uint64_t                      app_id;

   virtual void apply( const protocol::custom_json_operation& op ) ;
   virtual void apply( const protocol::custom_binary_operation & op ) { };

private:
   template<typename T> void save_message(const group_object& go, const account_name_type sender, bool system_message, const T& data);
   fc::sha256 extract_key( const std::map<public_key_type, encrypted_key>& new_key_map, const fc::sha256& group_key, const fc::sha256& iv, const public_key_type& sender_key);
};

message_wrapper decode_message( const vector<char>& message, const fc::sha256& iv, const fc::sha256& key )
{
   vector<char> raw_msg = fc::aes_decrypt(key, iv, message);
   message_wrapper ret;
   fc::raw::unpack_from_vector( raw_msg, ret );
   return ret;
}

vector<char> encode_message( const group_op& message, const fc::sha256& iv, const fc::sha256& key )
{
//   variant tmp;
//   to_variant( message, tmp );
   vector<char> raw_msg = fc::raw::pack_to_vector( message );
   return fc::aes_encrypt( key, iv, raw_msg);
}

message_wrapper decode_message( const vector<char>& message, const fc::sha512& key )
{
   vector<char> raw_msg = fc::aes_decrypt(key, message);
   message_wrapper ret;
   fc::raw::unpack_from_vector( raw_msg, ret );
   return ret;
}

vector<char> encode_message( const group_op& message, const fc::sha512& key )
{
//   variant tmp;
//   to_variant( message, tmp );
   vector<char> raw_msg = fc::raw::pack_to_vector( message );
   return fc::aes_encrypt( key, raw_msg);
}

template<typename T> void multiparty_messaging_plugin_impl::save_message(const group_object& go, const account_name_type sender, bool system_message, const T& data)
{
   _db.create<message_object>([&](message_object& mo){
        mo.group_name = go.group_name;
        mo.sequence = go.current_seq;
        mo.sender = sender;
        mo.recipients = go.members;
        mo.system_message = system_message;
        std::copy( data.begin(), data.end(), std::back_inserter(mo.data));
   });
   _db.modify(go, [&]( group_object& go){
        go.current_seq++;
   });
}

fc::sha256 multiparty_messaging_plugin_impl::extract_key( const std::map<public_key_type, encrypted_key>& new_key_map, const fc::sha256& group_key, const fc::sha256& iv, const public_key_type& sender_key)
{
   //first look for shared secret key
   for( const auto& pk: _private_keys ){
      const auto &nkm_itr =  new_key_map.find(pk.first);
      if(  nkm_itr != new_key_map.end() ){
         fc::sha512 sc = pk.second.get_shared_secret(sender_key);
         vector<char> key_data = fc::aes_decrypt( sc, nkm_itr->second );
         fc::sha256 key ( key_data.data(), key_data.size());
         return key;
      }
   }

   const auto &nkm_itr =  new_key_map.find(public_key_type());
   if(  nkm_itr != new_key_map.end() ){
      vector<char> key_data = fc::aes_decrypt( group_key, iv, nkm_itr->second );
      fc::sha256 key ( key_data.data(), key_data.size());
      return key;
   }

   return fc::sha256();
}

void multiparty_messaging_plugin_impl::apply( const protocol::custom_json_operation& op )
{
   account_name_type sender = op.sender;

   variant tmp = fc::json::from_string(&op.json[ 0 ]);
   group_meta message_meta = tmp.as<group_meta>();
   const auto& group_idx = _db.get_index< group_index >().indices().get< by_current_name >();

   if( message_meta.iv && message_meta.sender ) //message sent to the group
   {
      //check to which address this message came, and by whom
      for(account_name_type r: op.recipients){
         const auto& group_itr = group_idx.find(r);
         if( group_itr == group_idx.end() )
            continue;
         message_wrapper message_content = decode_message( message_meta.data, *message_meta.iv, group_itr->group_key);
         if( message_content.type == message_wrapper::message_type::group_operation ){
            //process_group_operation( *message_content.operation_data);
            FC_ASSERT(message_content.operation_data);
            group_op g_op = *message_content.operation_data;
            FC_ASSERT(g_op.version == 1);
            FC_ASSERT(op.sender == group_itr->admin);
            fc::sha256 new_key = extract_key( g_op.new_key, group_itr->group_key, *message_meta.iv, *message_meta.sender );
            if(g_op.type == "add"){ //other user added and group key changed; optionally group name changed
               _db.modify(*group_itr, [&](group_object& go) {
                    //go.members.merge_unique( g_op.user_list->begin(), g_op.user_list->end() );
                    go.members.clear();
                    std::copy( g_op.user_list->begin(), g_op.user_list->end(), std::back_inserter(go.members));
                    if(g_op.new_group_name)
                       go.current_group_name = *g_op.new_group_name;
                    go.group_key = new_key;
               });
            }else if(g_op.type == "delete"){// user deleted (can be us) and group key changed; optionally group name changed
               _db.modify(*group_itr, [&](group_object& go) {
                    go.members.clear();
                    std::copy( g_op.user_list->begin(), g_op.user_list->end(), std::back_inserter(go.members));
                    if(g_op.new_group_name)
                       go.current_group_name = *g_op.new_group_name;
                    go.group_key = new_key;
               });
            }else if(g_op.type == "disband"){ //current_name changed to "" and perticipant list emptied
               _db.modify(*group_itr, [&](group_object& go) {
                    go.members.clear();
                    if(g_op.new_group_name)
                       go.current_group_name = "";
                    go.group_key = fc::sha256();
               });
            }else if(g_op.type == "update"){//either group key or group name got updated...
               _db.modify(*group_itr, [&](group_object& go) {
                    if(g_op.new_group_name)
                       go.current_group_name = *g_op.new_group_name;
                    if(new_key != fc::sha256() )
                       go.group_key = new_key;
               });
            } else {
               FC_THROW("Unknown group operation type");
            }
            save_message<string>( *group_itr, op.sender, true, fc::json::to_string<group_op>(*message_content.operation_data));

         }else if( message_content.type == message_wrapper::message_type::message ){
            bool sender_in_member_list = false;
            for( const auto& m: group_itr->members )
               if ( m == op.sender ){ sender_in_member_list = true; break; }
            FC_ASSERT( sender_in_member_list );
            save_message<vector<char>>( *group_itr, op.sender, false, *message_content.message_data);
         } else {
            FC_ASSERT(true, "incorrect message_type");
         }
      }
   }
   else if (message_meta.sender && message_meta.recipient) //message sent to the recipient
   {
      const auto pk = _private_keys.find(*message_meta.recipient);
      FC_ASSERT( pk!= _private_keys.end() );
      fc::sha512 sc = pk.second.get_shared_secret(message_meta.sender);

      message_wrapper message_content = decode_message( message_meta.data, sc );
      FC_ASSERT( message_content.type == message_wrapper::message_type::group_operation );
      FC_ASSERT( message_content.operation_data );
      group_op g_op = *message_content.operation_data;
      FC_ASSERT( g_op.version == 1);
      //check that this group is new to us
      FC_ASSERT( g_op.new_group_name );
      const auto& group_itr = group_idx.find( *g_op.new_group_name);
      FC_ASSERT( group_itr == group_idx.end() );

      FC_ASSERT( g_op.type == "add" || g_op.type == "create" );
      fc::sha256 new_key = extract_key( g_op.new_key, fc::sha256(), fc::sha256(), *message_meta.sender );
      FC_ASSERT(new_key!=fc::sha256());
      _db.create<group_object>([&](group_object& go){
           go.group_name = go.current_group_name = g_op.new_group_name;
           go.members.clear();
           std::copy( g_op.user_list->begin(), g_op.user_list->end(), std::back_inserter(go.members));
           go.admin = op.sender;
           go.group_key = new_key;
      });
   }
}


} // detail

multiparty_messaging_plugin::multiparty_messaging_plugin() {}

void multiparty_messaging_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cfg.add_options()
         ("mpm-app-id", boost::program_options::value< long long >()->default_value( 2 ), "App id used by the multiparty messaging" )
         ("mpm-account", boost::program_options::value<vector<string>>()->composing()->multitoken(), "Accounts tracked by the plugin. If not specified, tries to listen to all messages within the given app ID")
         ("mpm-private-key", bpo::value<vector<string>>()->composing()->multitoken(), "WIF MEMO PRIVATE KEY to be used by one or more tracked accounts" )
   ;
}

void multiparty_messaging_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   if( options.count( "mpm-app-id" ) ){
      app_id = options[ "mpm-app-id" ].as< long long >();
   }else{
      ilog("App ID not given, multiparty messaging is disabled");
      return;
   }

   my = std::make_shared< detail::multiparty_messaging_plugin_impl >( *this );
   api = std::make_shared< multiparty_messaging_api >();

   if( options.count("mpm-account") ) {
      const std::vector<std::string>& accounts = options["mpm-account"].as<std::vector<std::string>>();
      for( const std::string& an:accounts ){
         account_name_type account(an);
         my->_accounts.insert(account);
      }
   }

   if( options.count("mpm-private-key") )
   {
      const std::vector<std::string> keys = options["mpm-private-key"].as<std::vector<std::string>>();
      for (const std::string& wif_key : keys )
      {
         fc::optional<fc::ecc::private_key> private_key = sophiatx::utilities::wif_to_key(wif_key);
         FC_ASSERT( private_key.valid(), "unable to parse private key" );
         my->_private_keys[private_key->get_public_key()] = *private_key;
      }
   }

   try
   {
      ilog( "Initializing multiparty_messaging_plugin_impl plugin" );
      chain::database& db = appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db();

      db.set_custom_operation_interpreter(app_id, dynamic_pointer_cast<custom_operation_interpreter, detail::multiparty_messaging_plugin_impl>(my));
      add_plugin_index< group_index >(db);
      add_plugin_index< message_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void multiparty_messaging_plugin::plugin_startup() {}

void multiparty_messaging_plugin::plugin_shutdown()
{
}

} } } // sophiatx::plugins::multiparty_messaging_plugin
