#include <sophiatx/plugins/track_and_trace/track_and_trace_plugin.hpp>

namespace sophiatx { namespace plugins { namespace track_and_trace_plugin {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

extern "C" BOOST_SYMBOL_EXPORT std::shared_ptr<abstract_plugin> plugin_factory() {
   return std::make_shared<track_and_trace_plugin>();
}

extern "C" BOOST_SYMBOL_EXPORT void options_setter(options_description& cli, options_description& cfg) {
   track_and_trace_plugin::set_program_options(cli, cfg);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif


} } } // sophiatx::plugins::track_and_trace_plugin
