#pragma once
#include <sophiatx/protocol/block_header.hpp>
#include <sophiatx/protocol/transaction.hpp>

namespace sophiatx { namespace protocol {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;

      uint32_t size();
   };

} } // sophiatx::protocol

FC_REFLECT_DERIVED( sophiatx::protocol::signed_block, (sophiatx::protocol::signed_block_header), (transactions) )
