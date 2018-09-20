
#pragma once

#include <memory>

namespace sophiatx { namespace schema {
   struct abstract_schema;
} }

namespace sophiatx { namespace protocol {
   struct custom_json_operation;
} }

namespace sophiatx { namespace chain {

class custom_operation_interpreter
{
   public:
      virtual void apply( const protocol::custom_json_operation& op ) = 0;
      virtual void apply( const protocol::custom_binary_operation & op ) = 0;
      //virtual std::shared_ptr< sophiatx::schema::abstract_schema > get_operation_schema() = 0;
};

} } // sophiatx::chain
