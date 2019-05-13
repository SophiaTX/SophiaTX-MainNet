#ifndef SOPHIATX_REMOTE_DB_HPP
#define SOPHIATX_REMOTE_DB_HPP

#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/network/http/websocket.hpp>

#include <string>
#include <vector>

namespace sophiatx {
namespace remote {
using namespace fc;

struct received_object {
   uint64_t id;
   std::string sender;
   std::vector<std::string> recipients;
   uint64_t app_id;
   std::string data;
   bool binary;
   fc::time_point_sec received;
};

struct get_app_custom_messages_args {
   uint64_t app_id;
   uint64_t start;
   uint32_t limit;
};

typedef std::map<uint64_t, received_object> get_app_custom_messages_return;

class remote_db {
public:
   inline static bool initialized() { return instance().remote_db_loaded_; }

   inline static void init(const std::string &endpoint) {
      FC_ASSERT(!instance().remote_db_loaded_, "remote_db already initialized!");
      instance().connnection_ = instance().ws_client_.connect(endpoint);
      instance().api_connection_ = std::make_shared<fc::rpc::websocket_api_connection>(*instance().connnection_);
      instance().closed_connection_ = (instance().connnection_->closed.connect([ = ] {
           elog("Server has disconnected us.");
           instance().remote_db_loaded_ = false;
      }));
      instance().remote_db_loaded_ = true;
   }

   inline static fc::variant remote_call(const std::string &api, const std::string call, const fc::variant &args) {
      FC_ASSERT(instance().remote_db_loaded_, "remote_db is not initialized!");
      return instance().api_connection_->send_call(api, call, true, {args});
   }

   inline static std::map<uint64_t, received_object>
   get_app_custom_messages(const get_app_custom_messages_args &args) {
      FC_ASSERT(instance().remote_db_loaded_, "remote_db is not initialized!");
      auto ret = instance().api_connection_->send_call("custom_api", "get_app_custom_messages", true, {fc::variant(args)});
      std::map<uint64_t, received_object> out;
      fc::from_variant(ret, out);
      return out;
   }

   remote_db(remote_db const &) = delete;

   void operator=(remote_db const &) = delete;

private:
   remote_db() : remote_db_loaded_(false), connnection_(nullptr) {}
   ~remote_db() {}

   inline static remote_db &instance() {
      static remote_db instance;
      return instance;
   }

   bool remote_db_loaded_;
   fc::http::websocket_client ws_client_;
   fc::http::websocket_connection_ptr connnection_;
   std::shared_ptr<fc::rpc::websocket_api_connection> api_connection_;
   boost::signals2::scoped_connection closed_connection_;
};

}
}

FC_REFLECT(sophiatx::remote::received_object, (id)(sender)(recipients)(app_id)(data)(received)(binary))
FC_REFLECT(sophiatx::remote::get_app_custom_messages_args, (app_id)(start)(limit))

#endif //SOPHIATX_REMOTE_DB_HPP
