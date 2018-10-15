//
// Created by fornadel on 11.10.2018.
//

#ifndef SOPHIATX_PUGINAPI_H
#define SOPHIATX_PUGINAPI_H

#include <string>


namespace sophiatx { namespace plugins { namespace test_plugin {


class PluginApi {
public:
   virtual std::string name() const = 0;

   virtual float calculate(float x, float y) = 0;

   virtual void initialize() = 0;

   virtual ~PluginApi() {}
};

} } }

#endif //SOPHIATX_PUGINAPI_H
