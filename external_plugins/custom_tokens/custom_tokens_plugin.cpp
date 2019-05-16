#include <sophiatx/plugins/custom_tokens/custom_tokens_plugin.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_objects.hpp>
#include <sophiatx/plugins/custom_tokens/custom_tokens_api.hpp>

#include <sophiatx/chain/database/database_interface.hpp>
#include <sophiatx/chain/index.hpp>
#include <sophiatx/chain/operation_notification.hpp>
#include <sophiatx/utilities/key_conversion.hpp>
#include <fc/io/raw.hpp>

#define NONE_SYMBOL_U64  (uint64_t('N') | (uint64_t('O') << 8) | (uint64_t('N') << 16) | (uint64_t('E') << 24))
#define NONE_SYMBOL_SER  (NONE_SYMBOL_U64)


namespace sophiatx {
namespace plugins {
namespace custom_tokens {

namespace detail {

class custom_tokens_plugin_impl {
public:
   custom_tokens_plugin_impl(custom_tokens_plugin &_plugin) :
         db_(appbase::app().get_plugin<sophiatx::plugins::chain::chain_plugin>().db()),
         self_(_plugin),
         app_id_(_plugin.app_id_) {}

   virtual ~custom_tokens_plugin_impl() {}

   void post_operation(const operation_notification &op_obj);

   const std::string
   save_token_error(const transaction_id_type &tx_id, const std::string &error,
                    asset_symbol_type token_symbol = NONE_SYMBOL_SER);

   void add_account_tokens(const account_name_type &name, asset_symbol_type token_symbol, uint64_t amount);

   void remove_account_tokens(const account_name_type &name, asset_symbol_type token_symbol, uint64_t amount,
                              const transaction_id_type &tx_id);

   void save_token_operation(const account_name_type &account, asset_symbol_type token_symbol,
                             const operation_notification &note);

   void check_if_paused(asset_symbol_type token_symbol, const transaction_id_type &tx_id);

   std::shared_ptr<database_interface> db_;
   custom_tokens_plugin &self_;

   uint64_t app_id_;
   uint32_t cleanup_day_ = 30;
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

         FC_ASSERT(tmp.get_object().contains("token_symbol"), "${s}",
                   ("s", plugin_.save_token_error(note_.trx_id, "Action field in json is missing!")));

         if( tmp[ "action" ].as<string>() == std::string("create_token")) {

            auto token_symbol = asset_symbol_type::from_string(tmp[ "token_symbol" ].as<string>());
            FC_ASSERT(token_symbol != NONE_SYMBOL_SER, "${s}",
                      ("s", plugin_.save_token_error(note_.trx_id, "This token symbol can not be used!",
                                                     token_symbol)));

            auto token = plugin_.db_->find<custom_token_object, by_token_symbol>(token_symbol);
            FC_ASSERT(!token, "${s}",
                      ("s", plugin_.save_token_error(note_.trx_id, "Token already exist!", token_symbol)));


            auto total_supply = tmp[ "total_supply" ].as<uint64_t>();

            plugin_.db_->create<custom_token_object>([ & ](custom_token_object &to) {
                 to.owner_name = from;
                 to.token_symbol = token_symbol;
                 to.total_supply = total_supply;
                 if( tmp.get_object().contains("max_supply"))
                    to.max_supply = tmp[ "max_supply" ].as<uint64_t>();
            });

            plugin_.add_account_tokens(from, token_symbol, total_supply);
            plugin_.save_token_operation(from, token_symbol, note_);

         } else if( tmp[ "action" ].as<string>() == std::string("pause_token")) {

            auto token_symbol = asset_symbol_type::from_string(tmp[ "token_symbol" ].as<string>());
            auto token = plugin_.db_->find<custom_token_object, by_token_symbol>(token_symbol);

            if( token->owner_name == from ) {
               plugin_.db_->modify(*token, [ & ](custom_token_object &to) {
                    to.paused ^= true;
               });

               plugin_.save_token_operation(from, token_symbol, note_);
            }

         } else if( tmp[ "action" ].as<string>() == std::string("transfer_token")) {

            auto amount = tmp[ "amount" ].as<asset>();

            plugin_.check_if_paused(amount.symbol, note_.trx_id);

            FC_ASSERT(op.recipients.size(), "${s}",
                      ("s", plugin_.save_token_error(note_.trx_id, "There is no recipient for tokens!",
                                                     amount.symbol)));

            plugin_.remove_account_tokens(from, amount.symbol, amount.amount.value, note_.trx_id);
            plugin_.add_account_tokens(*op.recipients.begin(), amount.symbol, amount.amount.value);

            plugin_.save_token_operation(from, amount.symbol, note_);
            plugin_.save_token_operation(*op.recipients.begin(), amount.symbol, note_);

         } else if( tmp[ "action" ].as<string>() == std::string("burn_token")) {

            auto amount = tmp[ "amount" ].as<asset>();

            plugin_.check_if_paused(amount.symbol, note_.trx_id);

            plugin_.remove_account_tokens(from, amount.symbol, amount.amount.value, note_.trx_id);
            plugin_.save_token_operation(from, amount.symbol, note_);

            auto token = plugin_.db_->find<custom_token_object, by_token_symbol>(amount.symbol);

            if( token ) {
               plugin_.db_->modify(*token, [ & ](custom_token_object &to) {
                    to.burned += amount.amount.value;
               });
            }

         } else if( tmp[ "action" ].as<string>() == std::string("issue_token")) {

            auto token_symbol = asset_symbol_type::from_string(tmp[ "token_symbol" ].as<string>());
            plugin_.check_if_paused(token_symbol, note_.trx_id);

            auto additional_amount = tmp[ "additional_amount" ].as<uint64_t>();
            auto token = plugin_.db_->find<custom_token_object, by_token_symbol>(token_symbol);

            FC_ASSERT(token, "${s}", ("s", plugin_.save_token_error(note_.trx_id, "No such a token!")));

            if( token->owner_name == from &&
                token->max_supply > token->total_supply + additional_amount ) {
               plugin_.db_->modify(*token, [ & ](custom_token_object &to) {
                    to.total_supply += additional_amount;
               });

               plugin_.add_account_tokens(from, token_symbol, additional_amount);
               plugin_.save_token_operation(from, token_symbol, note_);

            }

         } else {
            FC_ASSERT(false, "${s}",
                      ("s", plugin_.save_token_error(note_.trx_id, "Unknown action for custom token!")));
         }
      }
   }
};

void custom_tokens_plugin_impl::post_operation(const operation_notification &note) {
   note.op.visit(post_operation_visitor(*this, note));
}

void custom_tokens_plugin_impl::check_if_paused(asset_symbol_type token_symbol, const transaction_id_type &tx_id) {

   auto token = db_->find<custom_token_object, by_token_symbol>(token_symbol);
   FC_ASSERT(token, "${s}", ("s", save_token_error(tx_id, "No such a token!", token_symbol)));
   FC_ASSERT(!token->paused, "${s}",
             ("s", save_token_error(tx_id, "Can to do any action with token, because it is paused!", token_symbol)));
}

void custom_tokens_plugin_impl::add_account_tokens(const account_name_type &name, asset_symbol_type token_symbol,
                                                   uint64_t amount) {
   auto balance = db_->find<custom_token_account_object, by_token_and_account>(
         boost::make_tuple(token_symbol, name));
   if( balance ) {

      db_->modify(*balance, [ & ](custom_token_account_object &to) {
           to.amount += amount;
      });

   } else {

      uint64_t token_sequence = 1;
      uint64_t account_sequence = 1;
      {
         const auto &idx = db_->get_index<token_accounts_index>().indices().get<by_token_symbol>();
         auto itr = idx.lower_bound(boost::make_tuple(token_symbol, uint64_t(-1)));
         if( itr != idx.end() && itr->token_symbol == token_symbol )
            token_sequence = itr->token_sequence + 1;
      }

      {
         const auto &idx = db_->get_index<token_accounts_index>().indices().get<by_account>();
         auto itr = idx.lower_bound(boost::make_tuple(name, uint64_t(-1)));
         if( itr != idx.end() && itr->account_name == name )
            account_sequence = itr->account_sequence + 1;
      }

      db_->create<custom_token_account_object>([ & ](custom_token_account_object &to) {
           to.account_name = name;
           to.token_symbol = token_symbol;
           to.amount = amount;
           to.account_sequence = account_sequence;
           to.token_sequence = token_sequence;
      });
   }

}

void custom_tokens_plugin_impl::remove_account_tokens(const account_name_type &name, asset_symbol_type token_symbol,
                                                      uint64_t amount, const transaction_id_type &tx_id) {
   auto balance = db_->find<custom_token_account_object, by_token_and_account>(
         boost::make_tuple(token_symbol, name));
   FC_ASSERT(balance, "${s}",
             ("s", save_token_error(tx_id, "Account does not hold any of specified tokens!", token_symbol)));
   FC_ASSERT(balance->amount >= amount, "${s}",
             ("s", save_token_error(tx_id, "Account does not hold enought tokens!", token_symbol)));
   db_->modify(*balance, [ & ](custom_token_account_object &to) {
        to.amount -= amount;
   });
}

void
custom_tokens_plugin_impl::save_token_operation(const account_name_type &account, asset_symbol_type token_symbol,
                                                const operation_notification &note) {
   uint64_t token_sequence = 1;
   uint64_t account_sequence = 1;
   {
      const auto &idx = db_->get_index<token_operation_index>().indices().get<by_token_symbol>();
      auto itr = idx.lower_bound(boost::make_tuple(token_symbol, uint64_t(-1)));
      if( itr != idx.end() && itr->token_symbol == token_symbol )
         token_sequence = itr->token_sequence + 1;
   }

   {
      const auto &idx = db_->get_index<token_operation_index>().indices().get<by_account>();
      auto itr = idx.lower_bound(boost::make_tuple(account, uint64_t(-1)));
      if( itr != idx.end() && itr->account == account )
         account_sequence = itr->account_sequence + 1;
   }

   db_->create<custom_token_operation_object>([ & ](custom_token_operation_object &to) {
        to.account = account;
        to.token_symbol = token_symbol;
        to.trx_id = note.trx_id;
        to.block = note.block;
        to.timestamp = db_->head_block_time();
        to.account_sequence = account_sequence;
        to.token_sequence = token_sequence;
        auto size = fc::raw::pack_size(note.op);
        to.serialized_op.resize(size);
        fc::datastream<char *> ds(to.serialized_op.data(), size);
        fc::raw::pack(ds, note.op);
   });
}

const std::string custom_tokens_plugin_impl::save_token_error(const transaction_id_type &tx_id,
                                                              const std::string &error,
                                                              asset_symbol_type token_symbol) {

   uint64_t token_sequence = 1;
   {
      const auto &idx = db_->get_index<token_error_index>().indices().get<by_token_symbol>();
      auto itr = idx.lower_bound(boost::make_tuple(token_symbol, uint64_t(-1)));
      if( itr != idx.end() && itr->token_symbol == token_symbol )
         token_sequence = itr->token_sequence + 1;
   }

   db_->create<custom_token_error_object>([ & ](custom_token_error_object &cto) {
        cto.token_symbol = token_symbol;
        cto.trx_id = tx_id;
        cto.token_sequence = token_sequence;
        from_string(cto.error, error);
   });

   if( cleanup_day_ ) {
      // Clean up errors older then 30 days
      auto now = std::chrono::system_clock::now();
      const auto &seq_idx = db_->get_index<token_error_index, by_time>();
      auto seq_itr = seq_idx.lower_bound(now);
      vector<const custom_token_error_object *> to_remove;


      if( seq_itr == seq_idx.begin())
         return error;

      --seq_itr;

      while( now - seq_itr->time > std::chrono::hours(24 * cleanup_day_)) {
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
         ("custom-token-error-cleanup-days", boost::program_options::value<uint32_t>()->default_value(30),
          "Defines after how many days custom token errors will be discarded (0 means never)");
}

void custom_tokens_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
   if( options.count("custom-token-app-id")) {
      app_id_ = options[ "custom-token-app-id" ].as<uint64_t>();
   } else {
      ilog("App ID not given, custom tokens plugin is disabled");
      return;
   }

   my_ = std::make_shared<detail::custom_tokens_plugin_impl>(*this);
   api_ = std::make_shared<custom_tokens_api>(*this);

   if( options.count("custom-token-error-pruning")) {
      my_->cleanup_day_ = options[ "custom-token-error-pruning" ].as<uint32_t>();
   }

   try {
      ilog("Initializing custom_tokens_plugin_impl plugin");
      auto &db = app().get_plugin<sophiatx::plugins::chain::chain_plugin>().db();

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
