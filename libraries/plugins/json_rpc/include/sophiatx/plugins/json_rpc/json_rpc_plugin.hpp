#pragma once

#include <appbase/application.hpp>
#include <atomic>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>

#include <boost/config.hpp>
#include <boost/any.hpp>

/**
 * This plugin holds bindings for all APIs and their methods
 * and can dispatch JSONRPC requests to the appropriate API.
 *
 * For a plugin to use the API Register, it needs to specify
 * the register as a dependency. Then, during initializtion,
 * register itself using add_api.
 *
 * Ex.
 * appbase::app().get_plugin< json_rpc_plugin >().add_api(
 *    name(),
 *    {
 *       API_METHOD( method_1 ),
 *       API_METHOD( method_2 ),
 *       API_METHOD( method_3 )
 *    });
 *
 * All method should take a single struct as an argument called
 * method_1_args, method_2_args, method_3_args, etc. and should
 * return a single struct as a return type.
 *
 * For methods that do not require arguments, use api_void_args
 * as the argument type.
 */

#define SOPHIATX_JSON_RPC_PLUGIN_NAME "json_rpc"

#define JSON_RPC_REGISTER_API( API_NAME, APP )                                                       \
{                                                                                               \
   sophiatx::plugins::json_rpc::detail::register_api_method_visitor vtor( API_NAME, APP );              \
   for_each_api( vtor );                                                                        \
}

#define JSON_RPC_DEREGISTER_API( API_NAME, APP ) \
{ \
   sophiatx::plugins::json_rpc::detail::deregister_api( API_NAME, APP ); \
}

#define JSON_RPC_PARSE_ERROR        (-32700)
#define JSON_RPC_INVALID_REQUEST    (-32600)
#define JSON_RPC_METHOD_NOT_FOUND   (-32601)
#define JSON_RPC_INVALID_PARAMS     (-32602)
#define JSON_RPC_INTERNAL_ERROR     (-32603)
#define JSON_RPC_SERVER_ERROR       (-32000)
#define JSON_RPC_NO_PARAMS          (-32001)
#define JSON_RPC_PARSE_PARAMS_ERROR (-32002)
#define JSON_RPC_ERROR_DURING_CALL  (-32003)

namespace sophiatx { namespace plugins { namespace json_rpc {

using namespace appbase;

/**
 * @brief Internal type used to bind api methods
 * to names.
 *
 * Arguments: Variant object of propert arg type
 */
typedef std::function< fc::variant(const fc::variant&, const std::function<void( fc::variant&, uint64_t )>&, bool) > api_method;

/**
 * @brief An API, containing APIs and Methods
 *
 * An API is composed of several calls, where each call has a
 * name defined by the API class. The api_call functions
 * are compile time bindings of names to methods.
 */
typedef std::map< string, api_method > api_description;

struct api_method_signature
{
   fc::variant args;
   fc::variant ret;
};

namespace detail
{
   class json_rpc_plugin_impl;
}

class json_rpc_plugin : public appbase::plugin< json_rpc_plugin >
{
   public:
      static std::shared_ptr<json_rpc_plugin> &get_plugin() {
         static std::shared_ptr<json_rpc_plugin> instance(new json_rpc_plugin);
         return instance;
      }

      json_rpc_plugin(json_rpc_plugin const &) = delete;
      void operator=(json_rpc_plugin const &) = delete;


      virtual ~json_rpc_plugin();

      APPBASE_PLUGIN_REQUIRES();
      static void set_program_options( options_description&, options_description& );

      static const std::string& name() { static std::string name = SOPHIATX_JSON_RPC_PLUGIN_NAME; return name; }

      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      fc::optional< fc::variant > call_api_method(const string& network_name, const string& api_name, const string& method_name, const fc::variant& func_args, const std::function<void( fc::variant&, uint64_t )>& notify_callback) const;
      void add_api_method( const string& network_name, const string& api_name, const string& method_name, const api_method& api, const api_method_signature& sig );
      void remove_network_apis(const string& network_name, const string& api_name);

      string call( const string& body, bool& is_error);
      string call( const string& message, std::function<void(const string& )> callback);

      uint64_t generate_subscription_id();

      void set_default_network(const string& network_name);

   private:
      json_rpc_plugin();
      std::unique_ptr< detail::json_rpc_plugin_impl > my;
      std::atomic<uint64_t> _next_id;
};




struct ws_notice{
   string method="notice";
   std::pair<uint64_t, std::vector<fc::variant>> params;
};

namespace detail {

   class register_api_method_visitor
   {
      public:
         register_api_method_visitor( const std::string& api_name, application* app )
            : _api_name( api_name ),
              _json_rpc_plugin( sophiatx::plugins::json_rpc::json_rpc_plugin::get_plugin() ),
              _network_name( app->id )
         {
            ilog("registering api ${n}.${a}", ("n", _network_name)("a", _api_name));
         }

         template< typename Plugin, typename Method, typename Args, typename Ret >
         void operator()(
            Plugin& plugin,
            const std::string& method_name,
            Method method,
            Args* args,
            Ret* ret )
         {
            _json_rpc_plugin->add_api_method( _network_name, _api_name, method_name,
               [&plugin,method]( const fc::variant& args, const std::function<void( fc::variant&, uint64_t )>& notify_callback, bool lock = true ) -> fc::variant
               {
                  return fc::variant( (plugin.*method)( args.as< Args >(), notify_callback, lock ) );
               },
               api_method_signature{ fc::variant( Args() ), fc::variant( Ret() ) } );
         }

      private:
         std::string _api_name;
         std::shared_ptr<json_rpc_plugin> _json_rpc_plugin;
         std::string _network_name;

   };

   void deregister_api( const std::string& api, application* app );

}

} } } // sophiatx::plugins::json_rpc

namespace appbase {
using namespace sophiatx::plugins::json_rpc;
template<>
class plugin_factory<json_rpc_plugin> : public abstract_plugin_factory {
public:
   virtual ~plugin_factory() {}

   virtual std::shared_ptr<abstract_plugin> new_plugin() const {
      return json_rpc_plugin::get_plugin();
   }

   virtual void set_program_options(options_description &cli, options_description &cfg) {
      json_rpc_plugin::set_program_options(cli, cfg);
   };

   virtual std::string get_name() {
      return json_rpc_plugin::name();
   }
};
}


FC_REFLECT( sophiatx::plugins::json_rpc::api_method_signature, (args)(ret) )
FC_REFLECT( sophiatx::plugins::json_rpc::ws_notice, (method)(params))
