//
// Created by fornadel on 12.10.2018.
//

#ifndef SOPHIATX_TESTPLUGINAPI_H
#define SOPHIATX_TESTPLUGINAPI_H

#include <memory>

namespace sophiatx { namespace plugins { namespace test_plugin {

namespace detail
{
   class TestPluginApiImpl;
}

class TestPluginApi {
public:
   TestPluginApi();

   ~TestPluginApi();

//   DECLARE_API
//   ((get_current_holder)(get_holdings)(get_tracked_object_history)(get_transfer_requests)(get_item_details))

private:
   std::unique_ptr <detail::TestPluginApiImpl> my;
};

} } }

#endif //SOPHIATX_TESTPLUGINAPI_H
