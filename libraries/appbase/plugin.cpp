#include <appbase/plugin.hpp>
#include <appbase/application.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <fstream>

namespace appbase {

void  abstract_plugin::notify_app_initialize() {
   app()->plugin_initialized(*this);
}


void abstract_plugin::notify_app_startup() {
   app()->plugin_started(*this);
}

}