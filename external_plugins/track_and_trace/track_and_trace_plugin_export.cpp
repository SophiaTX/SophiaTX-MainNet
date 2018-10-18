#include <sophiatx/plugins/track_and_trace/track_and_trace_plugin.hpp>

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

extern "C" BOOST_SYMBOL_EXPORT std::shared_ptr<abstract_plugin> get_plugin() {
   return std::make_shared<track_and_trace_plugin>();
}

} } } // sophiatx::plugins::track_and_trace_plugin


