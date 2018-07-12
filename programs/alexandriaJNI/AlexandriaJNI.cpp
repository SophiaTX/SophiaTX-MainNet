#include "AlexandriaJNI.h"

#include <jni.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

#include <sophiatx/utilities/key_conversion.hpp>
#include <sophiatx/protocol/transaction.hpp>

#include <fc/io/json.hpp>
#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/api.hpp>


using namespace sophiatx::utilities;
using namespace sophiatx::protocol;
using namespace fc::ecc;
using namespace std;

struct Java_AlexandriaJNI_memo_data {

   static fc::optional<Java_AlexandriaJNI_memo_data> from_string( string str ) {
      try {
         if( str.size() > sizeof(Java_AlexandriaJNI_memo_data)) {
            auto data = fc::from_base58( str );
            auto m  = fc::raw::unpack_from_vector<Java_AlexandriaJNI_memo_data>( data );
            FC_ASSERT( string(m) == str );
            return m;
         }
      } catch ( ... ) {}
      return fc::optional<Java_AlexandriaJNI_memo_data>();
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

JNIEXPORT jobjectArray JNICALL Java_AlexandriaJNI_generateKeyPair(JNIEnv *env, jobject) {
   jobjectArray ret;
   ret = static_cast<jobjectArray>(env->NewObjectArray(2, env->FindClass("java/lang/String"), env->NewStringUTF("")));

   try {
      private_key_type priv_key = fc::ecc::private_key::generate();
      public_key_type pub_key = priv_key.get_public_key();
      auto public_key_str = fc::json::to_string(pub_key);

      env->SetObjectArrayElement(ret, 1, env->NewStringUTF(key_to_wif(priv_key).c_str()));
      env->SetObjectArrayElement(ret, 0, env->NewStringUTF(public_key_str.substr(1, public_key_str.size() - 2).c_str()));
      return ret;

    } catch (const fc::exception& e) {
       return ret;
    }
}

JNIEXPORT jobjectArray JNICALL Java_AlexandriaJNI_generateKeyPairFromBrainKey(JNIEnv *env, jobject,
                                                                              jstring inJNIStrBrainKey) {

   const char *brain_key = env->GetStringUTFChars(inJNIStrBrainKey, NULL);
   jobjectArray ret;
   ret = static_cast<jobjectArray>(env->NewObjectArray(2, env->FindClass("java/lang/String"), env->NewStringUTF("")));

   if (brain_key == NULL) {
      return ret;
   }

   try {
      fc::sha512 h = fc::sha512::hash(string(brain_key) + " 0");
      auto priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
      public_key_type pub_key = priv_key.get_public_key();
      auto public_key_str = fc::json::to_string(pub_key);

      env->SetObjectArrayElement(ret, 1, env->NewStringUTF(key_to_wif(priv_key).c_str()));
      env->SetObjectArrayElement(ret, 0, env->NewStringUTF(public_key_str.substr(1, public_key_str.size() - 2).c_str()));
      env->ReleaseStringUTFChars(inJNIStrBrainKey, brain_key);
      return ret;
   } catch (const fc::exception& e) {
      env->ReleaseStringUTFChars(inJNIStrBrainKey, brain_key);
      return ret;
   }

}

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_getPublicKey(JNIEnv *env, jobject, jstring inJNIStrPrivateKey) {
   const char *private_key = env->GetStringUTFChars(inJNIStrPrivateKey, NULL);

   if (private_key == NULL) {
      return NULL;
   }

   try {
      auto priv_key = *sophiatx::utilities::wif_to_key(string(private_key));
      env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
      public_key_type pub_key = priv_key.get_public_key();
      auto public_key_str = fc::json::to_string(pub_key);
      return env->NewStringUTF(public_key_str.substr(1, public_key_str.size() - 2).c_str());
   } catch (const fc::exception& e) {
      env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
      return NULL;
   }
}

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_getTransactionDigest(JNIEnv *env, jobject, jstring inJNIStrTransaction,
                                                                  jstring inJNIStrChainId) {

   const char *transaction = env->GetStringUTFChars(inJNIStrTransaction, NULL);
   if (transaction == NULL) {
      return NULL;
   }

   const char *chain_id = env->GetStringUTFChars(inJNIStrChainId, NULL);
   if (chain_id == NULL) {
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      return NULL;
   }

   try {
      string tx_str(transaction);
      fc::variant v = fc::json::from_string( tx_str, fc::json::strict_parser );
      signed_transaction stx;
      fc::from_variant( v, stx);
      digest_type dig = stx.sig_digest(fc::sha256(string(chain_id)));
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      env->ReleaseStringUTFChars(inJNIStrChainId, chain_id);
      return env->NewStringUTF(dig.str().c_str());
   } catch (const fc::exception& e) {
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      env->ReleaseStringUTFChars(inJNIStrChainId, chain_id);
      return NULL;
   }
}

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_signDigest(JNIEnv *env, jobject, jstring inJNIStrDigest,
                                                        jstring inJNIStrPrivateKey) {

   const char *digest = env->GetStringUTFChars(inJNIStrDigest, NULL);
   if (digest == NULL) {
      return NULL;
   }

   const char *private_key = env->GetStringUTFChars(inJNIStrPrivateKey, NULL);
   if (private_key == NULL) {
      env->ReleaseStringUTFChars(inJNIStrDigest, digest);
      return NULL;
   }
    try {
       fc::sha256 dig(string(digest, strlen(digest)));
       string private_k_str(private_key);
       auto priv_key = *sophiatx::utilities::wif_to_key(private_k_str);
       auto sig = priv_key.sign_compact(dig);
       string result = fc::json::to_string(sig);
       env->ReleaseStringUTFChars(inJNIStrDigest, digest);
       env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
       return env->NewStringUTF(result.substr(1, result.size() - 2).c_str());
    } catch (const fc::exception& e) {
       env->ReleaseStringUTFChars(inJNIStrDigest, digest);
       env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
       return NULL;
    }
 }

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_addSignature(JNIEnv *env, jobject, jstring inJNIStrTransaction,
                                                        jstring inJNIStrSignature) {

   const char *transaction = env->GetStringUTFChars(inJNIStrTransaction, NULL);
   if (transaction == NULL) {
      return NULL;
   }

   const char *signature = env->GetStringUTFChars(inJNIStrSignature, NULL);
   if (signature == NULL) {
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      return NULL;
   }

    try {
       string tx_str(transaction);
       fc::variant v = fc::json::from_string( tx_str, fc::json::strict_parser );
       signed_transaction stx;
       fc::from_variant( v, stx );

       compact_signature sig;
       fc::from_hex( string(signature), (char*)sig.begin(), sizeof(compact_signature) );

       stx.signatures.push_back(sig);

       env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
       env->ReleaseStringUTFChars(inJNIStrSignature, signature);
       return env->NewStringUTF(fc::json::to_string(stx).c_str());

    } catch (const fc::exception& e) {
       env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
       env->ReleaseStringUTFChars(inJNIStrSignature, signature);
       return NULL;
    }
 }

JNIEXPORT jboolean JNICALL Java_AlexandriaJNI_verifySignature(JNIEnv *env, jobject, jstring inJNIStrDigest,
                                                              jstring inJNIStrPublicKey, jstring inJNIStrSignature) {

   const char *digest = env->GetStringUTFChars(inJNIStrDigest, NULL);
   if (digest == NULL) {
      return static_cast<jboolean>(false);
   }

   const char *public_key = env->GetStringUTFChars(inJNIStrPublicKey, NULL);
   if (public_key == NULL) {
      env->ReleaseStringUTFChars(inJNIStrDigest, digest);
      return static_cast<jboolean>(false);
   }

   const char *signed_digest = env->GetStringUTFChars(inJNIStrSignature, NULL);
   if (signed_digest == NULL) {
      env->ReleaseStringUTFChars(inJNIStrDigest, digest);
      env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);
      return static_cast<jboolean>(false);
   }

    try {
       fc::sha256 dig(string(digest, strlen(digest)));

       fc::variant v = fc::json::from_string( string(public_key), fc::json::relaxed_parser );
       public_key_type pub_key;
       fc::from_variant( v, pub_key );

       compact_signature sig;
       fc::from_hex( string(signed_digest), (char*)sig.begin(), sizeof(compact_signature) );

       env->ReleaseStringUTFChars(inJNIStrDigest, digest);
       env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);
       env->ReleaseStringUTFChars(inJNIStrSignature, signed_digest);

       if(pub_key == fc::ecc::public_key(sig, dig)) {
          return static_cast<jboolean>(true);
       }

    } catch (const fc::exception& e) {
       env->ReleaseStringUTFChars(inJNIStrDigest, digest);
       env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);
       env->ReleaseStringUTFChars(inJNIStrSignature, signed_digest);
       return static_cast<jboolean>(false);
    }
   return static_cast<jboolean>(false);
 }

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_encryptMemo(JNIEnv *env, jobject, jstring inJNIStrMemo,
                                                         jstring inJNIStrPrivateKey, jstring inJNIStrPublicKey) {

   const char *memo = env->GetStringUTFChars(inJNIStrMemo, NULL);
   if (memo == NULL) {
      return NULL;
   }

   const char *private_key = env->GetStringUTFChars(inJNIStrPrivateKey, NULL);
   if (private_key == NULL) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      return NULL;
   }

   const char *public_key = env->GetStringUTFChars(inJNIStrPublicKey, NULL);
   if (public_key == NULL) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
      return NULL;
   }

    try {
       Java_AlexandriaJNI_memo_data m;

       auto priv_key = *sophiatx::utilities::wif_to_key(string(private_key));

       fc::variant v = fc::json::from_string( string(public_key), fc::json::relaxed_parser );
       public_key_type pub_key;
       fc::from_variant( v, pub_key );

       m.nonce = fc::time_point::now().time_since_epoch().count();

       auto shared_secret = priv_key.get_shared_secret( pub_key );

       fc::sha512::encoder enc;
       fc::raw::pack( enc, m.nonce );
       fc::raw::pack( enc, shared_secret );
       auto encrypt_key = enc.result();

       m.encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack_to_vector(string(memo)) );
       m.check = fc::sha256::hash( encrypt_key )._hash[0];

       env->ReleaseStringUTFChars(inJNIStrMemo, memo);
       env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
       env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);

       return env->NewStringUTF(string(m).c_str());

    } catch (const fc::exception& e) {
       env->ReleaseStringUTFChars(inJNIStrMemo, memo);
       env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
       env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);
       return NULL;
    }
 }

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_decryptedMemo(JNIEnv *env, jobject, jstring inJNIStrMemo,
                                                           jstring inJNIStrPrivateKey, jstring inJNIStrPublicKey) {

   const char *memo = env->GetStringUTFChars(inJNIStrMemo, NULL);
   if (memo == NULL) {
      return NULL;
   }

   const char *private_key = env->GetStringUTFChars(inJNIStrPrivateKey, NULL);
   if (private_key == NULL) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      return NULL;
   }

   const char *public_key = env->GetStringUTFChars(inJNIStrPublicKey, NULL);
   if (public_key == NULL) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
      return NULL;
   }

    try {
       string str_memo(memo);
       auto m = Java_AlexandriaJNI_memo_data::from_string( str_memo );

       if( m ) {
          fc::sha512 shared_secret;
          auto priv_key = *sophiatx::utilities::wif_to_key(string(private_key));

          fc::variant v = fc::json::from_string( string(public_key), fc::json::relaxed_parser );
          public_key_type pub_key;
          fc::from_variant( v, pub_key );

          shared_secret = priv_key.get_shared_secret(pub_key);

          fc::sha512::encoder enc;
          fc::raw::pack( enc, m->nonce );
          fc::raw::pack( enc, shared_secret );
          auto encryption_key = enc.result();

          uint64_t check = fc::sha256::hash( encryption_key )._hash[0];
          if( check != m->check ) {
             env->ReleaseStringUTFChars(inJNIStrMemo, memo);
             env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
             env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);
             return NULL;
          }

          vector<char> decrypted = fc::aes_decrypt( encryption_key, m->encrypted );

          env->ReleaseStringUTFChars(inJNIStrMemo, memo);
          env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
          env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);

          return env->NewStringUTF(fc::raw::unpack_from_vector<std::string>( decrypted ).c_str());
       } else {
          env->ReleaseStringUTFChars(inJNIStrMemo, memo);
          env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
          env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);
          return NULL;
       }
    } catch (const fc::exception& e) {
       env->ReleaseStringUTFChars(inJNIStrMemo, memo);
       env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
       env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);
       return NULL;
    }
 }

FC_REFLECT( Java_AlexandriaJNI_memo_data, (nonce)(check)(encrypted) )