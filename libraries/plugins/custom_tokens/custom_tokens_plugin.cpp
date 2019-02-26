#include <sophiatx/plugins/custom_tokens/custom_tokens_plugin.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_objects.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_api.hpp>

#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/index.hpp>
#include <sophiatx/chain/operation_notification.hpp>
#include <sophiatx/utilities/key_conversion.hpp>
#include <fc/io/raw.hpp>

namespace sophiatx {
namespace plugins {
namespace custom_tokens {

namespace detail {

class custom_tokens_plugin_impl {
public:
   custom_tokens_plugin_impl(custom_tokens_plugin &_plugin) :
         db_(_plugin.app()->get_plugin<sophiatx::plugins::chain::chain_plugin>().db()),
         self_(_plugin),
         app_id_(_plugin.app_id_) {}

   virtual ~custom_tokens_plugin_impl() {}

   void post_operation(const operation_notification &op_obj);

   const std::string
   save_token_error(const transaction_id_type &tx_id, const std::string &error, asset_symbol_type token_symbol = 0);

   std::shared_ptr<database_interface> db_;
   custom_tokens_plugin &self_;

   uint64_t app_id_;
   bool prune_ = true;
   boost::signals2::connection post_apply_connection;
};

struct post_operation_visitor {
   custom_tokens_plugin_impl &plugin_;
   const operation_notification &note_;

   post_operation_visitor(custom_tokens_plugin_impl &plugin, const operation_notification &note) : plugin_(plugin),
                                                                                                   note_(note) {}

   typedef void result_type;

   template<typename T>
   void operator()(const T &) const {}

   void operator()(const custom_json_operation &op) const {
      if( op.app_id == plugin_.app_id_ ) {
         account_name_type from = op.sender;
         FC_ASSERT(op.json.size(), "${s}",
                   ("s", plugin_.save_token_error(note_.trx_id, "Empty action for custom token!")));
         variant tmp = fc::json::from_string(&op.json[ 0 ]);

         if( tmp[ "action" ].as<string>() == std::string("create_token")) {
            plugin_.db_->create<custom_token_object>([ & ](custom_token_object &to) {
                 to.owner_name = from;
                 to.token_symbol = asset_symbol_type::from_string(tmp[ "token_symbol" ].as<string>());
                 to.total_supply = tmp[ "total_supply" ].as<uint64_t>();
                 if( tmp[ "max_supply" ].is_uint64())
                    to.max_supply = tmp[ "max_supply" ].as<uint64_t>();
            });
         } else if( tmp[ "action" ].as<string>() == std::string("pause_token")) {
            auto token_symbol = asset_symbol_type::from_string(tmp[ "token_symbol" ].as<string>());
            auto token = plugin_.db_->find<custom_token_object, by_token_symbol>(token_symbol);
            FC_ASSERT(token, "${s}", ("s", plugin_.save_token_error(note_.trx_id, "No such a token!")));
            if( token->owner_name == from ) {
               plugin_.db_->modify(*token, [ & ](custom_token_object &to) {
                    to.paused ^= true;
               });
            }
         } else if( tmp[ "action" ].as<string>() == std::string("transfer_token")) {
//            auto amount = tmp[ "amount" ].as<asset>();
            //TODo check balance change account balance
         } else if( tmp[ "action" ].as<string>() == std::string("burn_token")) {
//            auto amount = tmp[ "amount" ].as<asset>();
            //TODO check balance change account balance
         } else if( tmp[ "action" ].as<string>() == std::string("issue_token")) {
            auto token_symbol = asset_symbol_type::from_string(tmp[ "token_symbol" ].as<string>());
            auto additional_amount = tmp[ "additional_amount" ].as<uint64_t>();
            auto token = plugin_.db_->find<custom_token_object, by_token_symbol>(token_symbol);
            FC_ASSERT(token, "${s}", ("s", plugin_.save_token_error(note_.trx_id, "No such a token!")));
            if( token->owner_name == from && token->max_supply > token->total_supply + additional_amount ) {
               plugin_.db_->modify(*token, [ & ](custom_token_object &to) {
                    to.total_supply += additional_amount;
               });
               //TODO change_account_balance()
            }
         } else {
            FC_ASSERT(false, "${s}", ("s", plugin_.save_token_error(note_.trx_id, "Unknown action for custom token!")));
         }
      }
   }

};

void custom_tokens_plugin_impl::post_operation(const operation_notification &note) {
   note.op.visit(post_operation_visitor(*this, note));
}

//uint64_t custom_tokens_plugin_impl::get_account_balance(const account_name_type &name, asset_symbol_type token_symbol) {
//   return db_->find<custom_token_account_object, by_token_and_account>(boost::make_tuple(name, token_symbol));
//}
//
//void custom_tokens_plugin_impl::change_account_balance(const account_name_type &name, asset_symbol_type token_symbol) {
//   return db_->find<custom_token_account_object, by_token_and_account>(boost::make_tuple(name, token_symbol));
//}

//void custom_tokens_plugin_impl::save_token_operation(const account_name_type &account, bool system_message,
//                                                     const T &data) const {
//   db_->create<custom_token_operation_object>([ & ](custom_token_operation_object &cto) {
//        mo.group_name = go.group_name;
//        mo.sequence = go.current_seq;
//        mo.sender = sender;
//        mo.recipients = go.members;
//        mo.system_message = system_message;
//        std::copy(data.begin(), data.end(), std::back_inserter(mo.data));
//   });
//   _db->modify(go, [ & ](group_object &go) {
//        go.current_seq++;
//   });
//}

const std::string custom_tokens_plugin_impl::save_token_error(const transaction_id_type &tx_id,
                                                              const std::string &error,
                                                              asset_symbol_type token_symbol) {
   db_->create<custom_token_error_object>([ & ](custom_token_error_object &cto) {
        cto.token_symbol = token_symbol;
        cto.trx_id = tx_id;
        from_string(cto.error, error);
   });

   if( prune_ ) {
      // Clean up errors older then 30 days
      auto now = std::chrono::system_clock::now();
      const auto &seq_idx = db_->get_index<token_error_index, by_time>();
      auto seq_itr = seq_idx.lower_bound(now);
      vector<const custom_token_error_object *> to_remove;


      if( seq_itr == seq_idx.begin())
         return error;

      --seq_itr;

      while( now - seq_itr->time > std::chrono::hours(24 * 30)) {
         to_remove.push_back(&(*seq_itr));
         --seq_itr;
      }

      for( const auto *seq_ptr : to_remove ) {
         db_->remove(*seq_ptr);
      }
   }
   return error;
}

} // detail

custom_tokens_plugin::custom_tokens_plugin() {}

void custom_tokens_plugin::set_program_options(options_description &cli, options_description &cfg) {
   cfg.add_options()
         ("custom-token-app-id", boost::program_options::value<uint64_t>()->default_value(2),
          "App id used by the custom token plugin")
         ("custom-token-error-pruning", boost::program_options::value<bool>()->default_value(true),
          "Enable/Disable cleaning (more then 30 days) old custom token errors");
}

void custom_tokens_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
   if( options.count("custom-token-app-id")) {
      app_id_ = options[ "custom-token-app-id" ].as<uint64_t>();
   } else {
      ilog("App ID not given, multiparty messaging is disabled");
      return;
   }

   my_ = std::make_shared<detail::custom_tokens_plugin_impl>(*this);
   api_ = std::make_shared<custom_tokens_api>(*this);

   if( options.count("custom-token-error-pruning")) {
      my_->prune_ = options[ "custom-token-error-pruning" ].as<bool>();
   }

   try {
      ilog("Initializing custom_tokens_plugin_impl plugin");
      auto &db = app()->get_plugin<sophiatx::plugins::chain::chain_plugin>().db();

      my_->post_apply_connection = db->post_apply_operation.connect(0, [ & ](const operation_notification &o) {
           my_->post_operation(o);
      });

      add_plugin_index<token_index>(db);
      add_plugin_index<token_accounts_index>(db);
      add_plugin_index<token_operation_index>(db);
      add_plugin_index<token_error_index>(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void custom_tokens_plugin::plugin_startup() {}

void custom_tokens_plugin::plugin_shutdown() {
   chain::util::disconnect_signal(my_->post_apply_connection);
}

}
}
} // sophiatx::plugins::custom_tokens
