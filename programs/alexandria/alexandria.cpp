#include "alexandria.hpp"

#include <iostream>

#include <boost/algorithm/string.hpp>

#include <fc/io/json.hpp>
#include <fc/crypto/rand.hpp>

#include <steem/utilities/key_conversion.hpp>

#include <steem/protocol/transaction.hpp>

#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/api.hpp>


using namespace steem::utilities;
using namespace steem::protocol;
using namespace fc::ecc;
using namespace std;

bool generate_private_key(char *private_key) {
   try {
      private_key_type priv_key = fc::ecc::private_key::generate();
      strcpy(private_key, key_to_wif(priv_key).c_str());
      return true;
   } catch (const fc::exception& e) {
      return false;
   }
}

bool get_transaction_digest(const char *transaction, char *digest) {
   if(transaction) {
      try {
         string tx_str(transaction);
         fc::variant v = fc::json::from_string( tx_str, fc::json::strict_parser );
         signed_transaction stx;
         fc::from_variant( v, stx);
         digest_type dig = stx.digest();
         strcpy(digest, dig.str().c_str());
         return true;
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool sign_digest(const char *digest, const char *private_key, char *signed_digest) {
   if(digest && private_key)
   {
      try {
         fc::sha256 dig(string(digest, strlen(digest)));
         string private_k_str(private_key);
         auto priv_key = *steem::utilities::wif_to_key(private_k_str);
         auto sig = priv_key.sign_compact(dig);
         strcpy(signed_digest, fc::json::to_string(sig).c_str());
         return true;
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool add_signature(const char *transaction, const char *signature, char *signed_tx) {
   if(transaction && signature)
   {
      try {
         string tx_str(transaction);
         fc::variant v = fc::json::from_string( tx_str, fc::json::strict_parser );
         signed_transaction stx;
         fc::from_variant( v, stx );

         compact_signature sig;
         fc::from_hex( string(signature), (char*)sig.begin(), sizeof(compact_signature) );

         stx.signatures.push_back(sig);
         strcpy(signed_tx, fc::json::to_string(stx).c_str());
         return true;

      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

