#include <sophiatx/chain/database/hybrid_database.hpp>

#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>

namespace sophiatx {
namespace chain {



void hybrid_database::open(const open_args &args, const genesis_state_type &genesis,
          const public_key_type &init_pubkey /*TODO: delete when initminer pubkey is read from get_config */  ) {

   fc::http::websocket_client client;
   auto con  = client.connect( args.ws_endpoint );
   auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

   _remote_api = apic->get_remote_api< sophiatx::chain::remote_db_api >( 0, "alexandria_api", true /* forward api calls arguments as object */ );

}
optional<signed_block> hybrid_database::fetch_block_by_id(const block_id_type &id) const {
   return fetch_block_by_number(protocol::block_header::num_from_id(id));
}

optional<signed_block> hybrid_database::fetch_block_by_number(uint32_t num) const {
   get_block_args args;
   args.num = num;
   return _remote_api->get_block(std::move(args)).block;
}

}
}
