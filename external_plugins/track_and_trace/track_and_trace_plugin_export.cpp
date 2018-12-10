#include <sophiatx/plugins/track_and_trace/track_and_trace_plugin.hpp>

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

extern "C" BOOST_SYMBOL_EXPORT std::shared_ptr<abstract_plugin> get_plugin() {
   return std::make_shared<track_and_trace_plugin>();
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif


} } } // sophiatx::plugins::track_and_trace_plugin
