#include <sophiatx/plugins/test_api/test_api_plugin.hpp>

namespace sophiatx { namespace plugins { namespace test_api {

test_api_plugin::test_api_plugin() {}
test_api_plugin::~test_api_plugin() {}

void test_api_plugin::plugin_initialize( const variables_map& options )
{
   JSON_RPC_REGISTER_API( name() );
}

void test_api_plugin::plugin_startup() {}
void test_api_plugin::plugin_shutdown() {}

test_api_a_return test_api_plugin::test_api_a( const test_api_a_args& args, const std::function<void( fc::variant&, uint64_t )>& notify_callback, bool lock )
{
   FC_UNUSED( lock )
   test_api_a_return result;
   result.value = "A";
   return result;
}

test_api_b_return test_api_plugin::test_api_b( const test_api_b_args& args, const std::function<void( fc::variant&, uint64_t )>& notify_callback, bool lock )
{
   FC_UNUSED( lock )
   test_api_b_return result;
   result.value = "B";
   return result;
}

} } } // sophiatx::plugins::test_api
