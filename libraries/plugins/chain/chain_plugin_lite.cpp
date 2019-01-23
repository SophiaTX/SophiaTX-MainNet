#include <sophiatx/plugins/chain/chain_plugin_lite.hpp>

#include <sophiatx/chain/database/database_exceptions.hpp>
#include <sophiatx/chain/database/hybrid_database.hpp>

#include <sophiatx/remote_db/remote_db.hpp>


namespace sophiatx {
namespace plugins {
namespace chain {

using namespace sophiatx;
using sophiatx::chain::block_id_type;
namespace asio = boost::asio;

chain_plugin_lite::chain_plugin_lite() {
   flush_interval = 10000;
   db_ = std::make_shared<hybrid_database>();
}

chain_plugin_lite::~chain_plugin_lite() {}

void chain_plugin_lite::set_program_options(options_description &cli, options_description &cfg) {
   cfg.add_options()
         ("server-rpc-endpoint", bpo::value<string>()->default_value("ws://127.0.0.1:9191"),
          "Server websocket RPC endpoint")
         ("app-id", bpo::value<long long>()->default_value(1),
          "App id used by the hybrid DB")
         ("shared-file-dir", bpo::value<bfs::path>()->default_value("blockchain"),
          "the location of the chain shared memory files (absolute path or relative to application data dir)")
         ("shared-file-size", bpo::value<string>()->default_value("1G"), "Size of the shared memory file. Default: 1G.")
         ("shared-file-full-threshold", bpo::value<uint16_t>()->default_value(0),
          "A 2 precision percentage (0-10000) that defines the threshold for when to autoscale the shared memory file. Setting this to 0 disables autoscaling. Recommended value for consensus node is 9500 (95%). Full node is 9900 (99%)")
         ("shared-file-scale-rate", bpo::value<uint16_t>()->default_value(0),
          "A 2 precision percentage (0-10000) that defines how quickly to scale the shared memory file. When autoscaling occurs the file's size will be increased by this percent. Setting this to 0 disables autoscaling. Recommended value is between 1000-2000 (10-20%)");
   cli.add_options()
         ("resync-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and block log");
}

void chain_plugin_lite::plugin_initialize(const variables_map &options) {

   if( options.count("shared-file-dir")) {
      auto sfd = options.at("shared-file-dir").as<bfs::path>();
      if( sfd.is_relative())
         shared_memory_dir = app().data_dir() / sfd;
      else
         shared_memory_dir = sfd;
   }

   shared_memory_size = fc::parse_size(options.at("shared-file-size").as<string>());

   if( options.count("shared-file-full-threshold"))
      shared_file_full_threshold = options.at("shared-file-full-threshold").as<uint16_t>();

   if( options.count("shared-file-scale-rate"))
      shared_file_scale_rate = options.at("shared-file-scale-rate").as<uint16_t>();

   resync = options.at("resync-blockchain").as<bool>();

   ws_endpoint = options.at("server-rpc-endpoint").as<string>();
   app_id = options.at("app-id").as<long long>();
}

void chain_plugin_lite::plugin_startup() {
   ilog("Starting chain with shared_file_size: ${n} bytes", ("n", shared_memory_size));

   if( shared_memory_dir.generic_string().empty())
      shared_memory_dir = app().data_dir() / "blockchain";

   if( resync ) {
      wlog("resync requested: deleting block log and shared memory");
      db_->wipe(shared_memory_dir, true);
   }

   db_->set_flush_interval(flush_interval);

   database_interface::open_args db_open_args;
   db_open_args.shared_mem_dir = shared_memory_dir;
   db_open_args.shared_file_size = shared_memory_size;
   db_open_args.shared_file_full_threshold = shared_file_full_threshold;
   db_open_args.shared_file_scale_rate = shared_file_scale_rate;
   db_open_args.app_id = app_id;

   remote::remote_db::init(ws_endpoint);

   db_->open(db_open_args, genesis_state_type(), public_key_type());
}

void chain_plugin_lite::plugin_shutdown() {
   ilog("closing chain database");
   db_->close();
   ilog("database closed successfully");
}

}
}
} // namespace sophiatx::plugis::chain::chain_apis
