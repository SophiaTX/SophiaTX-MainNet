#pragma once

#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/block_api/block_api.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_args.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

class alexandria_api_impl {
public:
   alexandria_api_impl();
   ~alexandria_api_impl();

   /**
    * Getters and setters
    */
   chain::database &get_db() const;

   const shared_ptr<block_api::block_api> &get_block_api() const;
   void set_block_api(const shared_ptr<block_api::block_api> &block_api);


   /**
    * API methods declarations
    */
   DECLARE_API_IMPL(
         (get_block)
   )

   chain::database &_db;

private:
   std::shared_ptr<block_api::block_api> _block_api;
};

} } } // sophiatx::plugins::alexandria_api
