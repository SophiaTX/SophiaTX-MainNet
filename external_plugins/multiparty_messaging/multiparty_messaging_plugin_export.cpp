#include <sophiatx/plugins/multiparty_messaging/multiparty_messaging_plugin.hpp>

namespace sophiatx { namespace plugins { namespace multiparty_messaging {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

extern "C" BOOST_SYMBOL_EXPORT std::shared_ptr<abstract_plugin> plugin_factory() {
   return std::make_shared<multiparty_messaging_plugin>();
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif


} } } // sophiatx::plugins::multiparty_messaging
