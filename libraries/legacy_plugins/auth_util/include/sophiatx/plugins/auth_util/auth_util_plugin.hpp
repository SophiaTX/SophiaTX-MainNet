
#pragma once

#include <sophiatx/app/plugin.hpp>

namespace sophiatx { namespace plugin { namespace auth_util {

using sophiatx::app::application;

class auth_util_plugin : public sophiatx::app::plugin
{
   public:
      auth_util_plugin( application* app ) ;
      virtual ~auth_util_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;
};

} } }
