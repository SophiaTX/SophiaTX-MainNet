#include "alexandria.hpp"

#include <iostream>

#include <boost/algorithm/string.hpp>

#include <sophiatx/utilities/key_conversion.hpp>
#include <sophiatx/protocol/transaction.hpp>

#include <fc/io/json.hpp>
#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/api.hpp>


using namespace sophiatx::utilities;
using namespace sophiatx::protocol;
using namespace fc::ecc;
using namespace std;

namespace {

struct memo_data {

   static fc::optional<memo_data> from_string( string str ) {
      try {
         if( str.size() > sizeof(memo_data)) {
            auto data = fc::from_base58( str );
            auto m  = fc::raw::unpack_from_vector<memo_data>( data, 0 );
            FC_ASSERT( string(m) == str );
            return m;
         }
      } catch ( ... ) {}
      return fc::optional<memo_data>();
   }

   int64_t         nonce = 0;
   uint64_t        check = 0;
   vector<char>    encrypted;

   operator string()const {
      auto data = fc::raw::pack_to_vector(*this);
      auto base58 = fc::to_base58( data );
      return base58;
   }
};


bool generate_private_key(char *private_key, char *public_key) {
   try {
      private_key_type priv_key = fc::ecc::private_key::generate();
      public_key_type pub_key = priv_key.get_public_key();
      strcpy(private_key, key_to_wif(priv_key).c_str());
      auto public_key_str = fc::json::to_string(pub_key);
      strcpy(public_key, public_key_str.substr(1, public_key_str.size() - 2).c_str());
      return true;
   } catch (const fc::exception& e) {
      return false;
   }
}

bool get_public_key(const char *private_key, char *public_key) {
   if(private_key) {
      try {
         auto priv_key = sophiatx::utilities::wif_to_key(string(private_key));
         if(priv_key) {
            public_key_type pub_key = priv_key->get_public_key();
            auto public_key_str = fc::json::to_string(pub_key);
            strcpy(public_key, public_key_str.substr(1, public_key_str.size() - 2).c_str());
            return true;
         }
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
         public_key_type pub_key = priv_key.get_public_key();
         strcpy(private_key, key_to_wif(priv_key).c_str());
         auto public_key_str = fc::json::to_string(pub_key);
         strcpy(public_key, public_key_str.substr(1, public_key_str.size() - 2).c_str());
         return true;
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool get_transaction_digest(const char *transaction, const char *chain_id, char *digest) {
   if(transaction && chain_id) {
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
         auto priv_key = sophiatx::utilities::wif_to_key(private_k_str);
         if(priv_key) {
            auto sig = priv_key->sign_compact(dig, fc::ecc::bip_0062);
            string result = fc::json::to_string(sig);
            strcpy(signed_digest, result.substr(1, result.size() - 2).c_str());
            return true;
         }
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

         fc::variant v = fc::json::from_string( string(public_key), fc::json::relaxed_parser );
         public_key_type pub_key;
         fc::from_variant( v, pub_key );

         compact_signature sig;
         fc::from_hex( string(signed_digest), (char*)sig.begin(), sizeof(compact_signature) );

         if(pub_key == fc::ecc::public_key::recover_key(sig, dig, fc::ecc::bip_0062)) {
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

         auto priv_key = sophiatx::utilities::wif_to_key(string(private_key));

         if(priv_key) {
            fc::variant v = fc::json::from_string( string(public_key), fc::json::relaxed_parser );
            public_key_type pub_key;
            fc::from_variant( v, pub_key );

            m.nonce = fc::time_point::now().time_since_epoch().count();

            auto shared_secret = priv_key->get_shared_secret( pub_key );

            fc::sha512::encoder enc;
            fc::raw::pack( enc, m.nonce );
            fc::raw::pack( enc, shared_secret );
            auto encrypt_key = enc.result();

            m.encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack_to_vector(string(memo)) );
            m.check = fc::sha256::hash( encrypt_key )._hash[0];
            strcpy(encrypted_memo, string(m).c_str());
            return true;
         }

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
            auto priv_key = sophiatx::utilities::wif_to_key(string(private_key));
            if(priv_key) {
               fc::variant v = fc::json::from_string( string(public_key), fc::json::relaxed_parser );
               public_key_type pub_key;
               fc::from_variant( v, pub_key );

               shared_secret = priv_key->get_shared_secret(pub_key);

               fc::sha512::encoder enc;
               fc::raw::pack( enc, m->nonce );
               fc::raw::pack( enc, shared_secret );
               auto encryption_key = enc.result();

               uint64_t check = fc::sha256::hash( encryption_key )._hash[0];
               if( check != m->check ) return false;

               vector<char> decrypted = fc::aes_decrypt( encryption_key, m->encrypted );
               strcpy(decrypted_memo, fc::raw::unpack_from_vector<std::string>( decrypted, 0 ).c_str());

               return true;
            }
         }
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool get_shared_secret(const char *private_key, const char* public_key, char *shared_secret) {
   if(public_key && private_key) {
      try {
            fc::sha512 shared_sec;
            auto priv_key = sophiatx::utilities::wif_to_key(string(private_key));
            if(priv_key) {
               fc::variant v = fc::json::from_string( string(public_key), fc::json::relaxed_parser );
               public_key_type pub_key;
               fc::from_variant( v, pub_key );

               shared_sec = priv_key->get_shared_secret(pub_key);
               strcpy(shared_secret, shared_sec.str().c_str());

               return true;
         }
      } catch (const fc::exception& e) {
         return false;
      }
   }
   return false;
}

bool base64_decode(const char *input, char *output) {
   try {
      auto out = fc::base64_decode(string(input));
      strcpy(output, out.c_str());
   } catch (const fc::exception& e) {
      return false;
   }
   return true;
}

bool base64_encode(const char *input, char *output) {
   try {
      auto out = fc::base64_encode(string(input));
      strcpy(output, out.c_str());
   } catch (const fc::exception& e) {
      return false;
   }
   return true;
}

} //

FC_REFLECT( memo_data, (nonce)(check)(encrypted) )
