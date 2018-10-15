//
// Created by fornadel on 11.10.2018.
//

#ifndef SOPHIATX_TESTPLUGIN_H
#define SOPHIATX_TESTPLUGIN_H

#include <boost/config.hpp> // for BOOST_SYMBOL_EXPORT
#include <iostream>
#include <memory>

#include "PluginApi.hpp"

namespace sophiatx { namespace plugins { namespace test_plugin {

   class TestPlugin : public PluginApi {
   public:
      TestPlugin();

      // Overrides PluginApi methods
      std::string name() const override;

      float calculate(float x, float y) override;

      void initialize() override;

      ~TestPlugin();

      std::shared_ptr< class TestPluginApi > api;
   };

} } }

#endif //SOPHIATX_TESTPLUGIN_H
