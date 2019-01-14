#include<sophiatx/chain/database/remote_db_api.hpp>

namespace sophiatx { namespace chain {

// This class exists only to provide method signature information to fc::api, not to execute calls.

get_block_return remote_db_api::get_block( get_block_args args ) {
   FC_ASSERT( false );
}

} }
