#include <sophiatx/plugins/track_and_trace/track_and_trace_plugin.hpp>

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"

extern "C" BOOST_SYMBOL_EXPORT std::shared_ptr<abstract_plugin> get_plugin() {
   return std::make_shared<track_and_trace_plugin>();
}

#pragma GCC diagnostic pop

} } } // sophiatx::plugins::track_and_trace_plugin


