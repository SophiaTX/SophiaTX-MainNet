#include <sophiatx/chain/database/hybrid_database.hpp>
#include <sophiatx/chain/hybrid_db_property_object.hpp>
#include <sophiatx/chain/custom_operation_interpreter.hpp>
#include <sophiatx/chain/index.hpp>

#include <fc/network/http/websocket.hpp>

#define CUSTOM_API_SINGLE_QUERY_LIMIT 10000

namespace sophiatx {
namespace chain {

void hybrid_database::open(const open_args &args, const genesis_state_type &genesis,
                           const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  ) {
   try {


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
           elog("Server has disconnected us.");
           close();
      });

      add_core_index<hybrid_db_property_index>(shared_from_this());

      if( !find<hybrid_db_property_object>()) {
         create<hybrid_db_property_object>([ & ](hybrid_db_property_object &p) {
              p.head_op_number = 0;
         });
      }

      _head_op_number = get_hybrid_db_properties().head_op_number;

      if( _head_op_number == 0 ) {
         resync();
      } else {
         run();
      }

   }
   FC_CAPTURE_LOG_AND_RETHROW((args.shared_mem_dir)(args.shared_file_size))

}

void hybrid_database::close(bool /*rewind*/) {
   try {

      if( _remote_api_thread )
         _remote_api_thread->join();

      _remote_api_thread.reset();

      modify(get_hybrid_db_properties(), [ & ](hybrid_db_property_object &_hdpo) {
           _hdpo.head_op_number = _head_op_number;
      });

      chainbase::database::flush();
      chainbase::database::close();

      _closed_connection.disconnect();
   }
   FC_CAPTURE_AND_RETHROW()
}

void hybrid_database::resync() {
   do {
      auto results = _remote_api->get_app_custom_messages({_app_id, _head_op_number + CUSTOM_API_SINGLE_QUERY_LIMIT, CUSTOM_API_SINGLE_QUERY_LIMIT});

      if( results.empty())
         break;

      for( auto &&result : results ) {
         apply_custom_op(result.second);
      }
      _head_op_number += results.size();
   } while( true );

   run();
}

void hybrid_database::run() {
   _remote_api_thread = std::make_shared<std::thread>([ & ]() {
        auto results = _remote_api->get_app_custom_messages({_app_id, _head_op_number + CUSTOM_API_SINGLE_QUERY_LIMIT, CUSTOM_API_SINGLE_QUERY_LIMIT});
        for( auto &&result : results ) {
           apply_custom_op(result.second);
        }
        _head_op_number += results.size();

        boost::this_thread::sleep_for(boost::chrono::seconds(SOPHIATX_BLOCK_INTERVAL));
   });
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
