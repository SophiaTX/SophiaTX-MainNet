#pragma once
#include <sophiatx/plugins/json_rpc/utility.hpp>

#include <sophiatx/chain/custom_content_object.hpp>

#include <sophiatx/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>
#include <fc/crypto/base58.hpp>

namespace sophiatx { namespace plugins { namespace custom {


namespace detail { class custom_api_impl; }
using plugins::json_rpc::void_type;


struct received_object
{
   received_object() {};
   received_object( const sophiatx::chain::custom_content_object& obj ) :
         id(obj.id._id),
         sender( obj.sender ),
         app_id( obj.app_id ),
         binary( obj.binary ),
         received( obj.received)
   {
      if(binary)
         data = fc::base64_encode(obj.data.data(), obj.data.size());
      else
         data = obj.json;
      for(auto r: obj.all_recipients)
         recipients.push_back(r);
   }

   uint64_t          id;
   string            sender;
   vector<string>    recipients;
   uint64_t          app_id;
   string            data;
   bool              binary;
   time_point_sec    received;

};


struct list_received_documents_args
{
   uint32_t app_id;
   string   account_name;
   string   search_type; //"by_sender", "by_recipient", "by_sender_datetime", "by_recipient_datetime"
   string   start;
   uint32_t count;
};

struct get_received_document_args
{
   uint64_t id;
};

typedef std::map< uint64_t, received_object > list_received_documents_return;


typedef received_object get_received_document_return;

class custom_api
{
public:
   custom_api();
   ~custom_api();

   DECLARE_API(
         (list_received_documents)(get_received_document)
   )

private:
   std::unique_ptr< detail::custom_api_impl > my;
};

} } } // sophiatx::plugins::custom


FC_REFLECT( sophiatx::plugins::custom::received_object,
            (id)(sender)(recipients)(app_id)(data)(received)(binary) )

FC_REFLECT( sophiatx::plugins::custom::list_received_documents_args,
            (app_id)(account_name)(search_type)(start)(count) )

FC_REFLECT( sophiatx::plugins::custom::get_received_document_args,
            (id) )

