#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_plugin.hpp>
#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_objects.hpp>
#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_api.hpp>

#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/index.hpp>
#include <sophiatx/chain/custom_operation_interpreter.hpp>
#include <sophiatx/chain/operation_notification.hpp>
#include <sophiatx/utilities/key_conversion.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/io/raw.hpp>

namespace sophiatx { namespace plugins { namespace multiparty_messaging {

namespace detail {

class multiparty_messaging_plugin_impl : public custom_operation_interpreter
{
public:
   multiparty_messaging_plugin_impl( multiparty_messaging_plugin& _plugin ) :
         _db( appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ),
         app_id(_plugin.app_id) { }

   virtual ~multiparty_messaging_plugin_impl(){}
   std::shared_ptr<database_interface>  _db;
   multiparty_messaging_plugin&  _self;

   uint64_t                      app_id;

   virtual void apply( const protocol::custom_json_operation& op ) ;
   virtual void apply( const protocol::custom_binary_operation & op ) { };

private:
   template<typename T> void save_message(const group_object& go, const account_name_type sender, bool system_message, const T& data)const;
   fc::sha256 extract_key( const std::map<public_key_type, encrypted_key>& new_key_map, const fc::sha256& group_key, const fc::sha256& iv, const public_key_type& sender_key) const;
   const group_object* find_group(account_name_type name) const;
};

message_wrapper decode_message( const vector<char>& message, const fc::sha256& iv, const fc::sha256& key )
{
   vector<char> raw_msg = fc::aes_decrypt(key, iv, message);
   message_wrapper ret;
   fc::raw::unpack_from_vector( raw_msg, ret, 0 );
   return ret;
}


message_wrapper decode_message( const vector<char>& message, const fc::sha512& key )
{
   vector<char> raw_msg = fc::aes_decrypt(key, message);
   message_wrapper ret;
   fc::raw::unpack_from_vector( raw_msg, ret, 0 );
   return ret;
}

const group_object* multiparty_messaging_plugin_impl::find_group(account_name_type name) const
{
   return _db->find< group_object, by_current_name >( name );
}

template<typename T> void multiparty_messaging_plugin_impl::save_message(const group_object& go, const account_name_type sender, bool system_message, const T& data)const
{
   _db->create<message_object>([&](message_object& mo){
        mo.group_name = go.group_name;
        mo.sequence = go.current_seq;
        mo.sender = sender;
        mo.recipients = go.members;
        mo.system_message = system_message;
        std::copy( data.begin(), data.end(), std::back_inserter(mo.data));
   });
   _db->modify(go, [&]( group_object& go){
        go.current_seq++;
   });
}

fc::sha256 multiparty_messaging_plugin_impl::extract_key( const std::map<public_key_type, encrypted_key>& new_key_map, const fc::sha256& group_key, const fc::sha256& iv, const public_key_type& sender_key) const
{
   //first look for shared secret key
   for( const auto& pk: _self._private_keys ){
      const auto &nkm_itr =  new_key_map.find(pk.first);
      if(  nkm_itr != new_key_map.end() ){
         fc::sha512 sc = pk.second.get_shared_secret(sender_key);
         vector<char> key_data = fc::aes_decrypt( sc, nkm_itr->second );
         fc::sha256 key( key_data.data(), key_data.size());
         return key;
      }
      //in case we are the sender, there won't be our key in the map... Let's try this:
      if( pk.first == sender_key) {
         auto nkm_itr = new_key_map.begin();
         while( nkm_itr!= new_key_map.end() && nkm_itr->first == public_key_type())
            nkm_itr++;
         if( nkm_itr== new_key_map.end() )
            continue;
         fc::sha512 sc = pk.second.get_shared_secret(nkm_itr->first);
         vector<char> key_data = fc::aes_decrypt(sc, nkm_itr->second);
         fc::sha256 key(key_data.data(), key_data.size());
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
   group_meta message_meta = fc::json::from_string(&op.json[ 0 ]).as<group_meta>();

   if( message_meta.iv ) //message sent to the group
   {
      //check to which address this message came, and by whom
      for(account_name_type r: op.recipients){
         const group_object* g_ob = find_group(r);
         if( !g_ob ) continue;

         message_wrapper message_content = decode_message( message_meta.data, *message_meta.iv, g_ob->group_key);
         if( message_content.type == message_wrapper::message_type::group_operation ){
            //process_group_operation( *message_content.operation_data);
            FC_ASSERT(message_content.operation_data, "Message has incorrect format");
            group_op g_op = *message_content.operation_data;
            FC_ASSERT(g_op.version == 1 && op.sender == g_ob->admin, "Wrong version or wrong admin");
            fc::sha256 new_key = extract_key( g_op.new_key, g_ob->group_key, *message_meta.iv, g_op.senders_pubkey );

            if(g_op.type == "disband"){ //current_name changed to "" and perticipant list emptied
               _db->modify(*g_ob, [&](group_object& go) {
                    go.members.clear();
                    go.current_group_name = "";
                    go.group_key = fc::sha256();
               });
            }else if( g_op.type == "update" ){
               _db->modify(*g_ob, [&](group_object& go) {
                    //TODO - save the delta in user lists so we can store it in save_message
                    go.members.clear();
                    std::copy( g_op.user_list->begin(), g_op.user_list->end(), std::back_inserter(go.members));
                    if(g_op.new_group_name)      go.current_group_name = *g_op.new_group_name;
                    if(new_key != fc::sha256() ) go.group_key = new_key;
                    if(g_op.description.size())  from_string( go.description, g_op.description);
               });
            } else {
               FC_THROW("Unknown group operation type");
            }
            save_message<string>( *g_ob, op.sender, true, fc::json::to_string<group_op>(*message_content.operation_data));

         }else if( message_content.type == message_wrapper::message_type::message ){
            save_message<vector<char>>( *g_ob, op.sender, false, *message_content.message_data);
         } else {
            FC_ASSERT(true, "incorrect message_type");
         }
      }
      return;
   }

   if (message_meta.sender && message_meta.recipient) //message sent to the recipient
   {
      vector<account_name_type> involved_accounts;
      std::copy(op.recipients.begin(), op.recipients.end(), std::back_inserter(involved_accounts));
      involved_accounts.push_back(op.sender);
      for( account_name_type r: involved_accounts ){
         if( _self._accounts.find(r) != _self._accounts.end()) {
            auto pk_r = _self._private_keys.find(*message_meta.recipient);
            auto pk_s = _self._private_keys.find(*message_meta.sender);
            fc::sha512 shared_secret;
            if( pk_r != _self._private_keys.end())
               shared_secret = pk_r->second.get_shared_secret(*message_meta.sender);
            else if( pk_s != _self._private_keys.end())
               shared_secret = pk_s->second.get_shared_secret(*message_meta.recipient);
            else
               return;

            message_wrapper message_content = decode_message(message_meta.data, shared_secret);
            FC_ASSERT(message_content.type == message_wrapper::message_type::group_operation &&
                      message_content.operation_data);
            group_op g_op = *message_content.operation_data;
            FC_ASSERT(g_op.version == 1 && g_op.type == "add");
            //check that this group is new to us
            FC_ASSERT(g_op.new_group_name);
            if( find_group(*g_op.new_group_name)) return;

            fc::sha256 new_key = extract_key(g_op.new_key, fc::sha256(), fc::sha256(), *message_meta.sender);
            FC_ASSERT(new_key != fc::sha256());
            const auto &g_ob = _db->create<group_object>([ & ](group_object &go) {
                 go.group_name = *g_op.new_group_name;
                 go.current_group_name = go.group_name;
                 go.members.clear();
                 std::copy(g_op.user_list->begin(), g_op.user_list->end(), std::back_inserter(go.members));
                 go.admin = op.sender;
                 go.group_key = new_key;
                 from_string(go.description, g_op.description);
            });
            save_message<string>(g_ob, op.sender, true, fc::json::to_string<group_op>(*message_content.operation_data));
         }
      }
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

   _my = std::make_shared< detail::multiparty_messaging_plugin_impl >( *this );
   api = std::make_shared< multiparty_messaging_api >(*this);

   if( options.count("mpm-account") ) {
      const std::vector<std::string>& accounts = options["mpm-account"].as<std::vector<std::string>>();
      for( const std::string& an:accounts ){
         account_name_type account(an);
         _accounts.insert(account);
      }
   }

   if( options.count("mpm-private-key") )
   {
      const std::vector<std::string> keys = options["mpm-private-key"].as<std::vector<std::string>>();
      for (const std::string& wif_key : keys )
      {
         fc::optional<fc::ecc::private_key> private_key = sophiatx::utilities::wif_to_key(wif_key);
         FC_ASSERT( private_key.valid(), "unable to parse private key" );
         _private_keys[private_key->get_public_key()] = *private_key;
      }
   }

   try
   {
      ilog( "Initializing multiparty_messaging_plugin_impl plugin" );
      auto& db = appbase::app().get_plugin< sophiatx::plugins::chain::chain_plugin >().db();

      db->set_custom_operation_interpreter(app_id, dynamic_pointer_cast<custom_operation_interpreter, detail::multiparty_messaging_plugin_impl>(_my));
      add_plugin_index< group_index >(db);
      add_plugin_index< message_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void multiparty_messaging_plugin::plugin_startup() {
   api->api_startup();
}

void multiparty_messaging_plugin::plugin_shutdown() {}

} } } // sophiatx::plugins::multiparty_messaging
