#pragma once

#include <sophiatx/plugins/json_rpc/utility.hpp>
#include <sophiatx/plugins/alexandria_api/alexandria_api_args.hpp>

#define ALEXANDRIA_API_SINGLE_QUERY_LIMIT 1000

namespace sophiatx { namespace plugins { namespace alexandria_api {

class alexandria_api_impl;

class alexandria_api
{
   public:
      alexandria_api();
      ~alexandria_api();

      void init();

      DECLARE_API(

         /**
         * @brief Returns the information about a block
         *
         * @param num Block num
         *
         * @returns Public block data on the blockchain
         */
         (get_block)

      )

   private:
      std::unique_ptr< alexandria_api_impl > my;
};

} } } //sophiatx::plugins::alexandria_api

