#ifndef SOPHIATX_REMOTE_DB_API_HPP
#define SOPHIATX_REMOTE_DB_API_HPP

#include <sophiatx/chain/sophiatx_objects.hpp>
#include <sophiatx/chain/account_object.hpp>
#include <sophiatx/chain/block_summary_object.hpp>
#include <sophiatx/chain/global_property_object.hpp>
#include <sophiatx/chain/hardfork_property_object.hpp>
#include <sophiatx/chain/history_object.hpp>
#include <sophiatx/chain/sophiatx_objects.hpp>
#include <sophiatx/chain/witness_objects.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>


namespace sophiatx { namespace chain {

using namespace fc;
using namespace std;

struct received_object
{
   uint64_t          id;
   string            sender;
   vector<string>    recipients;
   uint64_t          app_id;
   string            data;
   bool              binary;
   time_point_sec    received;
};

struct get_app_custom_messages_args
{
   uint64_t app_id;
   uint64_t start;
   uint32_t limit;
};

typedef std::map<uint64_t, received_object> get_app_custom_messages_return;

/**
 * This is a dummy API so that the wallet can create properly formatted API calls
 */
struct remote_db_api
{
   get_app_custom_messages_return get_app_custom_messages( chain::get_app_custom_messages_args args );
};


} }

FC_API( sophiatx::chain::remote_db_api,
              (get_app_custom_messages)
)

FC_REFLECT( sophiatx::chain::received_object,
            (id)(sender)(recipients)(app_id)(data)(received)(binary) )
            
FC_REFLECT( sophiatx::chain::get_app_custom_messages_args,
            (app_id)(start)(limit) )

#endif //SOPHIATX_REMOTE_DB_API_HPP
