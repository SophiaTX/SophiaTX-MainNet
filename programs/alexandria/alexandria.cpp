#include "alexandria.hpp"

#include <iostream>

#include <boost/algorithm/string.hpp>

#include <steem/utilities/key_conversion.hpp>
#include <steem/protocol/transaction.hpp>

#include <fc/io/json.hpp>
#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/api.hpp>


using namespace steem::utilities;
using namespace steem::protocol;
using namespace fc::ecc;
using namespace std;

struct memo_data {

   static fc::optional<memo_data> from_string( string str ) {
      try {
         if( str.size() > sizeof(memo_data)) {
            auto data = fc::from_base58( str );
            auto m  = fc::raw::unpack_from_vector<memo_data>( data );
            FC_ASSERT( string(m) == str );
            return m;
         }
      } catch ( ... ) {}
      return fc::optional<memo_data>();
   }

   public_key_type from;
   public_key_type to;
   uint64_t        nonce = 0;
   uint32_t        check = 0;
   vector<char>    encrypted;

   operator string()const {
      auto data = fc::raw::pack_to_vector(*this);
      auto base58 = fc::to_base58( data );
      return base58;
   }
};

FC_REFLECT( memo_data, (from)(to)(nonce)(check)(encrypted) )

bool generate_private_key(char *private_key, char *public_key) {
   try {
      private_key_type priv_key = fc::ecc::private_key::generate();
      public_key_type pub_key = priv_key.get_public_key();
      strcpy(private_key, key_to_wif(priv_key).c_str());
      strcpy(public_key, public_key::to_base58( pub_key ).c_str());
      return true;
   } catch (const fc::exception& e) {
      return false;
   }
}

bool get_public_key(const char *private_key, char *public_key) {
   if(private_key) {
      try {
         auto priv_key = *steem::utilities::wif_to_key(string(private_key));
         auto pub_key = priv_key.get_public_key();
         strcpy(public_key, public_key::to_base58(pub_key).c_str());
         return true;
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool generate_key_pair_from_brain_key(const char *brain_key, char *private_key, char *public_key) {
   if(brain_key) {
      try {
         fc::sha512 h = fc::sha512::hash(string(brain_key) + " 0");
         auto priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
         auto pub_key = priv_key.get_public_key();
         strcpy(private_key, key_to_wif(priv_key).c_str());
         strcpy(public_key, public_key::to_base58(pub_key).c_str());
         return true;
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool get_transaction_digest(const char *transaction, const char *chain_id, char *digest) {
   if(transaction) {
      try {
         string tx_str(transaction);
         fc::variant v = fc::json::from_string( tx_str, fc::json::strict_parser );
         signed_transaction stx;
         fc::from_variant( v, stx);
         digest_type dig = stx.sig_digest(fc::sha256(string(chain_id)));
         strcpy(digest, dig.str().c_str());
         return true;
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool sign_digest(const char *digest, const char *private_key, char *signed_digest) {
   if(digest && private_key) {
      try {
         fc::sha256 dig(string(digest, strlen(digest)));
         string private_k_str(private_key);
         auto priv_key = *steem::utilities::wif_to_key(private_k_str);
         auto sig = priv_key.sign_compact(dig);
         string result = fc::json::to_string(sig);
         strcpy(signed_digest, result.substr(1, result.size() - 1).c_str());
         return true;
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool add_signature(const char *transaction, const char *signature, char *signed_tx) {
   if(transaction && signature) {
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

bool verify_signature(const char *digest, const char *public_key, const char *signed_digest) {
   if(digest && public_key && signed_digest) {
      try {
         fc::sha256 dig(string(digest, strlen(digest)));
         auto pub_key = public_key::from_base58(string(public_key));
         compact_signature sig;
         fc::from_hex( string(signed_digest), (char*)sig.begin(), sizeof(compact_signature) );

         if(pub_key == fc::ecc::public_key(sig, dig)) {
            return true;
         }

      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}


bool encrypt_memo(const char *memo, const char *private_key, const char *public_key, char *encrypted_memo) {
   if(memo && private_key && public_key) {
      try {
         memo_data m;

         auto priv_key = *steem::utilities::wif_to_key(string(private_key));

         m.from = priv_key.get_public_key();
         m.to = public_key::from_base58(string(public_key));
         m.nonce = fc::time_point::now().time_since_epoch().count();

         auto shared_secret = priv_key.get_shared_secret( m.to );

         fc::sha512::encoder enc;
         fc::raw::pack( enc, m.nonce );
         fc::raw::pack( enc, shared_secret );
         auto encrypt_key = enc.result();

         m.encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack_to_vector(string(memo)) );
         m.check = fc::sha256::hash( encrypt_key )._hash[0];
         strcpy(encrypted_memo, string(m).c_str());
         return true;

      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool decrypt_memo(const char *memo, const char *private_key, const char* public_key, char *decrypted_memo) {
   if(memo && private_key) {
      try {
         string str_memo(memo);
         auto m = memo_data::from_string( str_memo );

         if( m ) {
            fc::sha512 shared_secret;
            auto priv_key = *steem::utilities::wif_to_key(string(private_key));
            shared_secret = priv_key.get_shared_secret(public_key::from_base58(string(public_key)));

            fc::sha512::encoder enc;
            fc::raw::pack( enc, m->nonce );
            fc::raw::pack( enc, shared_secret );
            auto encryption_key = enc.result();

            uint32_t check = fc::sha256::hash( encryption_key )._hash[0];
            if( check != m->check ) return false;

            vector<char> decrypted = fc::aes_decrypt( encryption_key, m->encrypted );
            strcpy(decrypted_memo, fc::raw::unpack_from_vector<std::string>( decrypted ).c_str());

            return true;
         }
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}


