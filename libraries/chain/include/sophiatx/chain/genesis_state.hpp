/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <sophiatx/protocol/types.hpp>

#include <fc/crypto/sha256.hpp>
#include <sophiatx/protocol/config.hpp>

#include <string>
#include <vector>

namespace sophiatx { namespace chain {
using std::string;
using std::vector;
using namespace sophiatx::protocol;

struct genesis_state_type {

   struct initial_account_type {
      initial_account_type(const account_name_type& name = string(),
                           const public_key_type& key = public_key_type(),
                           const share_type& balance = 0
      )
            : name(name),
              key(key),
              balance(balance)
      {}
      account_name_type name;
      public_key_type key;
      share_type balance;
   };

   public_key_type                          initial_public_key = SOPHIATX_INIT_PUBLIC_KEY;
   share_type                               initial_balace = SOPHIATX_INIT_SUPPLY;

   time_point_sec                           initial_timestamp = SOPHIATX_GENESIS_TIME;
   vector<initial_account_type>             initial_accounts;


   /**
    * Temporary, will be moved elsewhere.
    */
   chain_id_type                            initial_chain_id = SOPHIATX_CHAIN_ID;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const{return initial_chain_id;};
};

} } // namespace sophiatx::chain

FC_REFLECT(sophiatx::chain::genesis_state_type::initial_account_type, (name)(key)(balance))

FC_REFLECT(sophiatx::chain::genesis_state_type, (initial_public_key)(initial_balace)(initial_timestamp)(initial_accounts)(initial_chain_id))