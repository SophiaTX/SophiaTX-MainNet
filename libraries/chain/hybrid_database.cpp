#include <sophiatx/chain/database/hybrid_database.hpp>
#include <sophiatx/chain/hybrid_db_property_object.hpp>
#include <sophiatx/chain/custom_operation_interpreter.hpp>
#include <sophiatx/chain/index.hpp>

namespace sophiatx {
namespace chain {

void hybrid_database::open(const open_args &args, const genesis_state_type &genesis) {
   try {

      FC_ASSERT(remote::remote_db::initialized(), "Remoted DB for hybrid DB is not initialized!");

      chainbase::database::open(args.shared_mem_dir, args.chainbase_flags, args.shared_file_size);

      _shared_file_full_threshold = args.shared_file_full_threshold;
      _shared_file_scale_rate = args.shared_file_scale_rate;
      _app_id = args.app_id;

      add_core_index<hybrid_db_property_index>(shared_from_this());
      _plugin_index_signal();

      with_write_lock([ & ]() {

           if( !find<hybrid_db_property_object>()) {
              create<hybrid_db_property_object>([ & ](hybrid_db_property_object &p) {
                   p.head_op_number = 0;
                   p.head_op_id = 0;
              });
           }

           _head_op_number = get_hybrid_db_properties().head_op_number;
           _head_op_id = get_hybrid_db_properties().head_op_id;
      });

      _running = true;

      start_sync_with_full_node();

   }
   FC_CAPTURE_LOG_AND_RETHROW((args.shared_mem_dir)(args.shared_file_size))

}

void hybrid_database::close(bool /*rewind*/) {
   try {
      _running = false;

      with_write_lock([ & ]() {
           modify(get_hybrid_db_properties(), [ & ](hybrid_db_property_object &_hdpo) {
                _hdpo.head_op_number = _head_op_number;
                _hdpo.head_op_id = _head_op_id;
           });
      });

      chainbase::database::flush();
      chainbase::database::close();

      boost::this_thread::sleep_for(boost::chrono::seconds(chain::sophiatx_config::get<uint32_t>("SOPHIATX_BLOCK_INTERVAL")));
      if( _remote_api_thread.is_running())
         _remote_api_thread.quit();

   }
   FC_CAPTURE_AND_RETHROW()
}


bool hybrid_database::is_sync() const {
   if( _head_op_id == _head_op_number && _head_op_number == 0 )
      return false;

   auto result = remote::remote_db::get_app_custom_messages({_app_id, std::numeric_limits<uint64_t>::max(), 1});

   if( result.empty() || result.begin()->second.id != _head_op_id )
      return false;

   return true;
}

const remote::get_app_custom_messages_return::iterator &
hybrid_database::get_unprocessed_op(const remote::get_app_custom_messages_return::iterator &start,
                                    const remote::get_app_custom_messages_return::iterator &end, size_t size) const {

   if( start->second.id > _head_op_id || start == end )
      return start;

   auto mid = start;

   std::advance(mid, size / 2);
   if( std::distance(mid, end) > static_cast<int64_t>(size / 2))
      return start;

   if( mid->second.id == _head_op_id ) {
      return ++mid;
   } else if( mid->second.id > _head_op_id ) {
      return get_unprocessed_op(start, --mid, size / 2);
   } else {
      return get_unprocessed_op(++mid, end, size / 2);
   }
}

void hybrid_database::start_sync_with_full_node() {
   _remote_api_thread.async([ & ]() {
                                 while( _running ) {
                                    do {
                                       if( is_sync())
                                          break;

                                       auto results = remote::remote_db::get_app_custom_messages(
                                             {_app_id, _head_op_number + SOPHIATX_API_SINGLE_QUERY_LIMIT,
                                              SOPHIATX_API_SINGLE_QUERY_LIMIT});

                                       if( results.empty())
                                          break;

                                       auto itr = results.begin();

                                       if( _head_op_id != 0 )
                                          itr = get_unprocessed_op(results.begin(), results.end(), results.size());

                                       if( itr == results.end())
                                          break;

                                       for( ; itr != results.end(); ++itr ) {
                                          if( itr->second.id > _head_op_id || itr->second.id == 0 ) {
                                             apply_custom_op(itr->second);
                                             _head_op_number++;
                                             _head_op_id = itr->second.id;
                                          }
                                       }
                                    } while( true );
                                    boost::this_thread::sleep_for(boost::chrono::seconds(chain::sophiatx_config::get<uint32_t>("SOPHIATX_BLOCK_INTERVAL")));
                                 }
                            }

   );
}

void hybrid_database::apply_custom_op(const remote::received_object &obj) {
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
         with_write_lock([ & ]() {
              eval->apply(op);
         });
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
         with_write_lock([ & ]() {
              eval->apply(op);
         });
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
