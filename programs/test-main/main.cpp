#include <iostream>
#include <boost/dll/import.hpp>

#include "../../external_plugins/test_plugin/include/sophiatx/plugins/test_plugin/PluginApi.hpp"

int main() {
   std::cout << "Start test" << std::endl;


   try {
      const std::string name = "../../external_plugins/test_plugin/libtest_external_plugin.so";
      boost::filesystem::path lib_path(name);                 // argv[1] contains path to directory with our plugin library
      boost::shared_ptr<sophiatx::plugins::test_plugin::PluginApi> plugin;   // variable to hold a pointer to plugin variable
      std::cout << "Loading the plugin" << name << std::endl;

      plugin = boost::dll::import<sophiatx::plugins::test_plugin::PluginApi>(          // type of imported symbol is located between `<` and `>`
            name,                     // path to the library and library name
            "plugin",                                       // name of the symbol to import
            boost::dll::load_mode::append_decorations              // makes `libmy_plugin_sum.so` or `my_plugin_sum.dll` from `my_plugin_sum`
      );

      //plugin->initialize(my->_args);
      plugin->initialize();
   }
   catch (const std::exception& e) {
      std::cout << "Exception: " << e.what() << std::endl;
   }


   std::cout << "End test" << std::endl;
   return 0;
}