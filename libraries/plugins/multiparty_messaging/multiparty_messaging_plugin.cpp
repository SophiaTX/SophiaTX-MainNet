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

};

group_op decode_message( const vector<char>& message, const fc::sha256& iv, const fc::sha256& key )
{
   vector<char> raw_msg = fc::aes_decrypt(key, iv, message);
   group_op ret;
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

void multiparty_messaging_plugin_impl::apply( const protocol::custom_json_operation& op )
{
   account_name_type sender = op.sender;



//   FC_ASSERT(op.recipients.size());
//   account_name_type serial = *op.recipients.begin();
//   FC_ASSERT(op.json.size());
//   variant tmp = fc::json::from_string(&op.json[ 0 ]);
//   FC_ASSERT( (std::string)serial == tmp[ "serial" ].as<string>() );
//
//   if( tmp[ "action" ].as<string>() == std::string("register")) {
//      _db.create<possession_object>([&](possession_object& po){
//         po.holder="";
//         po.new_holder = "";
//         po.serial = serial;
//         from_string(po.meta, tmp["meta"].as<string>());
//         from_string(po.claim_key, tmp["claimKey"].as<string>());
//      });//*/
//   } else {
//      //read the db entry here
//      const auto& holder_idx = _db.get_index< posession_index >().indices().get< by_serial >();
//      const auto& holder_itr = holder_idx.find(serial);
//      FC_ASSERT(holder_itr != holder_idx.end(), "Item with given number not found");
//      if( tmp[ "action" ].as<string>() == std::string("claim")) {
//         FC_ASSERT(tmp["claimKey"].as<string>() == to_string(holder_itr->claim_key), "incorrect claim key");
//         _db.modify(*holder_itr,[&](possession_object& po){
//              po.holder = sender;
//              from_string(po.info, tmp["info"].as<string>());
//         });
//      } else if( tmp[ "action" ].as<string>() == std::string("updateInfo")) {
//         FC_ASSERT(sender == holder_itr->holder);
//         _db.modify(*holder_itr,[&](possession_object& po){
//              from_string(po.info, tmp["info"].as<string>());
//         });
//      } else if( tmp[ "action" ].as<string>() == std::string("handoverRequest")) {
//         FC_ASSERT(sender == holder_itr->holder);
//         _db.modify(*holder_itr,[&](possession_object& po){
//              po.new_holder = tmp["newOwner"].as<string>();
//         });
//      } else if( tmp[ "action" ].as<string>() == std::string("handoverAck")) {
//         FC_ASSERT(sender == holder_itr->new_holder);
//         _db.modify(*holder_itr,[&](possession_object& po){
//              po.new_holder = "";
//              po.holder = sender;
//         });
//         _db.create<transfer_history_object>([&](transfer_history_object& o){
//              o.new_holder = sender;
//              o.serial = serial;
//              o.change_date = fc::time_point::now();
//         });
//      } else if( tmp[ "action" ].as<string>() == std::string("handoverReject")) {
//         FC_ASSERT(sender == holder_itr->new_holder);
//         _db.modify(*holder_itr,[&](possession_object& po){
//              po.new_holder = "";
//         });
//
//      }
//   }
}


} // detail

multiparty_messaging_plugin::multiparty_messaging_plugin() {}

void multiparty_messaging_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cfg.add_options()
         ("mpm-app-id", boost::program_options::value< long long >()->default_value( 2 ), "App id used by the multiparty messaging" )
         ("mpm-account", boost::program_options::value<vector<string>>()->composing()->multitoken(), "Accounts tracked by the plugin. If not specified, tries to listen to all messages within the given app ID")
         ("mpm-private-key", bpo::value<vector<string>>()->composing()->multitoken(), "WIF PRIVATE KEY to be used by one or more tracked accounts" )
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
