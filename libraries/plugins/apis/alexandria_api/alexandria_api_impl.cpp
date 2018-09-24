
#include <sophiatx/plugins/alexandria_api/alexandria_api_impl.hpp>
#include <sophiatx/plugins/chain/chain_plugin.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

alexandria_api_impl::alexandria_api_impl()
      : _db(appbase::app().get_plugin<sophiatx::plugins::chain::chain_plugin>().db()) {}

alexandria_api_impl::~alexandria_api_impl() {}

chain::database &alexandria_api_impl::get_db() const {
   return _db;
}

const shared_ptr<block_api::block_api> &alexandria_api_impl::get_block_api() const {
   return _block_api;
}

void alexandria_api_impl::set_block_api(const shared_ptr<block_api::block_api> &block_api) {
   _block_api = block_api;
}



/**
 *  Api methods implementations
 */
DEFINE_API_IMPL(alexandria_api_impl, get_block) {
   FC_ASSERT(_block_api, "block_api_plugin not enabled.");
   return _block_api->get_block( {args.block_num} );
}



} } } // sophiatx::plugins::alexandria_api

