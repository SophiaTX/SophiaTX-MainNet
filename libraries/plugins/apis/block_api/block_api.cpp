#include <appbase/application.hpp>

#include <sophiatx/plugins/block_api/block_api.hpp>
#include <sophiatx/plugins/block_api/block_api_plugin.hpp>

#include <sophiatx/protocol/get_config.hpp>
#include <algorithm>

namespace sophiatx { namespace plugins { namespace block_api {

class block_api_impl
{
   public:
      block_api_impl(block_api_plugin& plugin);
      ~block_api_impl();

      DECLARE_API_IMPL(
         (get_block_header)
         (get_block)
         (get_average_block_size)
      )

   std::shared_ptr<chain::database_interface> _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

block_api::block_api(block_api_plugin& plugin)
   : my( new block_api_impl(plugin) )
{
   JSON_RPC_REGISTER_API( SOPHIATX_BLOCK_API_PLUGIN_NAME, plugin.app() );
}

block_api::~block_api() {}

block_api_impl::block_api_impl(block_api_plugin& plugin)
   : _db( plugin.app()->get_plugin< sophiatx::plugins::chain::chain_plugin >().db() ) {}

block_api_impl::~block_api_impl() {}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////
DEFINE_API_IMPL( block_api_impl, get_block_header )
{
   get_block_header_return result;
   auto block = _db->fetch_block_by_number( args.block_num );

   if( block )
      result.header = *block;

   return result;
}

DEFINE_API_IMPL( block_api_impl, get_block )
{
   get_block_return result;
   auto block = _db->fetch_block_by_number( args.block_num );

   if( block )
      result.block = *block;

   return result;
}

DEFINE_API_IMPL( block_api_impl, get_average_block_size )
{
   int64_t start = _db->head_block_num();
   int64_t stop = std::max( start - 1000, 0ll );
   uint64_t total_size = 0;
   for( int64_t i = start; i > stop; i-- )
      total_size += fc::raw::pack_size( _db->fetch_block_by_number( i ) );
   return total_size / (start - stop );
}

DEFINE_READ_APIS( block_api,
   (get_block_header)
   (get_block)
   (get_average_block_size)
)

} } } // sophiatx::plugins::block_api
