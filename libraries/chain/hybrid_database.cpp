#include <sophiatx/chain/database/hybrid_database.hpp>
#include <sophiatx/chain/hybrid_db_property_object.hpp>
#include <sophiatx/chain/custom_operation_interpreter.hpp>
#include <sophiatx/chain/index.hpp>


#include <fc/network/http/websocket.hpp>

namespace sophiatx {
namespace chain {

void hybrid_database::open(const open_args &args, const genesis_state_type &genesis,
                           const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  ) {
   try {
      ilog("initializing database...");

      chainbase::database::open(args.shared_mem_dir, args.chainbase_flags, args.shared_file_size);
      _shared_file_full_threshold = args.shared_file_full_threshold;
      _shared_file_scale_rate = args.shared_file_scale_rate;
      _app_id = args.app_id;

      fc::http::websocket_client client;
      auto con = client.connect(args.ws_endpoint);
      _apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

      _remote_api = _apic->get_remote_api<sophiatx::chain::remote_db_api>(0, "custom_api",
                                                                          true /* forward api calls arguments as object */ );

      _closed_connection = con->closed.connect([ = ] {
           cerr << "Server has disconnected us.\n";
           close();
      });

      add_core_index<hybrid_db_property_index>(shared_from_this());

      if( !find<hybrid_db_property_object>()) {
         create<hybrid_db_property_object>([ & ](hybrid_db_property_object &p) {
              p.head_op_number = 0;
         });
      }

      _head_op_number = get_hybrid_db_properties().head_op_number;

      run();
   }
   FC_CAPTURE_LOG_AND_RETHROW((args.shared_mem_dir)(args.shared_file_size))

}

void hybrid_database::close(bool /*rewind*/) {
   try {
      modify(get_hybrid_db_properties(), [ & ](hybrid_db_property_object &_hdpo) {
           _hdpo.head_op_number = _head_op_number;
      });

      chainbase::database::flush();
      chainbase::database::close();

      _closed_connection.disconnect();
   }
   FC_CAPTURE_AND_RETHROW()
}

uint32_t hybrid_database::reindex(const open_args &args, const genesis_state_type &genesis,
                                  const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  ) {

   uint32_t last_block_number = 0; // result
   try {
      ilog("Reindexing Blockchain");
      wipe(args.shared_mem_dir, false);
      open(args, genesis, init_pubkey);

      auto start = fc::time_point::now();

      ilog("Replaying blocks...");

      run();

      auto end = fc::time_point::now();
      ilog("Done reindexing, elapsed time: ${t} sec", ("t", double((end - start).count()) / 1000000.0));

      return last_block_number;
   }
   FC_CAPTURE_AND_RETHROW((args.shared_mem_dir))
}

void hybrid_database::run() {
   auto results = _remote_api->get_app_custom_messages({_app_id, _head_op_number + 1000, 1000});
   for( auto &&result : results ) {
      apply_custom_op(result.second);
   }
}

void hybrid_database::apply_custom_op(const received_object &obj) {
   auto eval = get_custom_json_evaluator(obj.app_id);

   if( !eval )
      return;

   if( obj.binary ) {
      custom_binary_operation op;
      op.app_id = obj.app_id;
      op.sender = obj.sender;
      for( const auto &r: obj.recipients )
         op.recipients.insert(r);
      auto out = fc::base64_decode(obj.data);
      std::copy(out.begin(), out.end(), std::back_inserter(op.data));

      try {
         eval->apply(op);
      }
      catch( const fc::exception &e ) {
         edump((e));
      }
      catch( ... ) {
         elog("Unexpected exception applying custom json evaluator.");
      }

   } else {

      custom_json_operation op;
      op.app_id = obj.app_id;
      op.sender = obj.sender;
      for( const auto &r: obj.recipients )
         op.recipients.insert(r);
      op.json = obj.data;

      try {
         eval->apply(op);
      }
      catch( const fc::exception &e ) {
         edump((e));
      }
      catch( ... ) {
         elog("Unexpected exception applying custom json evaluator.");
      }
   }
}

const hybrid_db_property_object &hybrid_database::get_hybrid_db_properties() const {
   try {
      return get<hybrid_db_property_object>();
   } FC_CAPTURE_AND_RETHROW()
}


}
}
