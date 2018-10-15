//
// Created by fornadel on 11.10.2018.
//

#include <sophiatx/plugins/test_plugin/TestPlugin.hpp>
#include <sophiatx/plugins/test_plugin/TestPluginApi.hpp>

namespace sophiatx { namespace plugins { namespace test_plugin {

TestPlugin::TestPlugin() {
   std::cout << "Constructing TestPlugin" << std::endl;
}

void TestPlugin::initialize() {
   std::cout << "TestPlugin::initialize()" << std::endl;
   api = std::make_shared< TestPluginApi >();
}

std::string TestPlugin::name() const {
   return "sum";
}

float TestPlugin::calculate(float x, float y) {
   return x + y;
}

TestPlugin::~TestPlugin() {
   std::cout << "Destructing TestPlugin" << std::endl;
}

// Exporting `my_namespace::plugin` variable with alias name `plugin`
// (Has the same effect as `BOOST_DLL_ALIAS(my_namespace::plugin, plugin)`)
extern "C" BOOST_SYMBOL_EXPORT TestPlugin plugin;
TestPlugin plugin;

} } }