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

   static std::optional<Java_AlexandriaJNI_memo_data> from_string( string str ) {
      try {
         if( str.size() > sizeof(Java_AlexandriaJNI_memo_data)) {
            auto data = fc::from_base58( str );
            auto m  = fc::raw::unpack_from_vector<Java_AlexandriaJNI_memo_data>( data, 0 );
            FC_ASSERT( string(m) == str );
            return m;
         }
      } catch ( ... ) {}
      return std::optional<Java_AlexandriaJNI_memo_data>();
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

JNIEXPORT jbyteArray JNICALL Java_AlexandriaJNI_generatePrivateKey(JNIEnv *env, jclass) {
   try {
      private_key_type priv_key = fc::ecc::private_key::generate();

      std::vector<char> key_bytes = fc::variant(priv_key).as<std::vector<char>>();

      jbyteArray ret = env->NewByteArray(static_cast<jsize>(key_bytes.size()));

      if (env->ExceptionCheck() || ret == nullptr) {
         return nullptr;
      }
      env->SetByteArrayRegion(ret, 0, static_cast<jsize>(key_bytes.size()), reinterpret_cast<jbyte*>(key_bytes.data()));

      if (env->ExceptionCheck()) {
         env->ReleaseByteArrayElements(ret, reinterpret_cast<jbyte*>(key_bytes.data()), 0);
         return nullptr;
      }
      return ret;
    } catch (const fc::exception& e) {
       return nullptr;
    }
}

JNIEXPORT jbyteArray JNICALL Java_AlexandriaJNI_generatePrivateKeyFromBrainKey(JNIEnv *env, jclass,
                                                                               jstring inJNIStrBrainKey) {

   const char *brain_key = env->GetStringUTFChars(inJNIStrBrainKey, NULL);
   if (brain_key == nullptr || env->ExceptionCheck()) {
      return nullptr;
   }

   try {
      fc::sha512 h = fc::sha512::hash(string(brain_key) + " 0");
      env->ReleaseStringUTFChars(inJNIStrBrainKey, brain_key);

      auto priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));

      std::vector<char> key_bytes = fc::variant(priv_key).as<std::vector<char>>();

      jbyteArray ret = env->NewByteArray(static_cast<jsize>(key_bytes.size()));

      if (env->ExceptionCheck() || ret == nullptr) {
         return nullptr;
      }
      env->SetByteArrayRegion(ret, 0, static_cast<jsize>(key_bytes.size()), reinterpret_cast<jbyte*>(key_bytes.data()));

      if (env->ExceptionCheck()) {
         env->ReleaseByteArrayElements(ret, reinterpret_cast<jbyte*>(key_bytes.data()), 0);
         return nullptr;
      }
      return ret;

   } catch (const fc::exception& e) {
      env->ReleaseStringUTFChars(inJNIStrBrainKey, brain_key);
      return nullptr;
   }

}

JNIEXPORT jbyteArray JNICALL Java_AlexandriaJNI_getPublicKey(JNIEnv *env, jclass, jbyteArray private_key) {
   try {
      int length = env->GetArrayLength(private_key);
      if(length == -1) {
         return nullptr;
      }

      std::vector<char> private_k((size_t)length);
      env->GetByteArrayRegion(private_key, 0, length, reinterpret_cast<jbyte*>(&private_k.front()));

      if (env->ExceptionCheck()) {
         return nullptr;
      }

      fc::ecc::private_key key = fc::variant(private_k).as<fc::ecc::private_key>();

      public_key_type pub_key = key.get_public_key();

      public_key_type::binary_key k;
      k.data = pub_key.key_data;
      k.check = fc::ripemd160::hash( k.data.data, static_cast<uint32_t>(k.data.size()) )._hash[0];
      auto data = fc::raw::pack_to_vector( k );

      jbyteArray ret = env->NewByteArray(static_cast<jsize>(data.size()));

      if (env->ExceptionCheck() || ret == nullptr) {
         return nullptr;
      }
      env->SetByteArrayRegion(ret, 0, static_cast<jsize>(data.size()), reinterpret_cast<jbyte*>(data.data()));

      if (env->ExceptionCheck()) {
         env->ReleaseByteArrayElements(ret, reinterpret_cast<jbyte*>(data.data()), 0);
         return nullptr;
      }

      return ret;
   } catch (const fc::exception& e) {
      return nullptr;
   }
}

JNIEXPORT jbyteArray JNICALL Java_AlexandriaJNI_getTransactionDigest(JNIEnv *env, jclass, jstring inJNIStrTransaction,
                                                                     jbyteArray inJNIChainID) {

   const char *transaction = env->GetStringUTFChars(inJNIStrTransaction, 0);
   if (transaction == nullptr) {
      return nullptr;
   }

   int length = env->GetArrayLength(inJNIChainID);
   if(length == -1) {
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      return nullptr;
   }

   std::vector<char> chain_id((size_t)length);
   env->GetByteArrayRegion(inJNIChainID, 0, length, reinterpret_cast<jbyte*>(&chain_id.front()));

   if (env->ExceptionCheck()) {
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      return nullptr;
   }

   try {
      string tx_str(transaction);
      fc::variant v = fc::json::from_string( tx_str, fc::json::strict_parser );
      signed_transaction stx;
      fc::from_variant( v, stx);
      digest_type dig = stx.sig_digest(fc::sha256(chain_id.data(), chain_id.size()));
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);

      jbyteArray ret = env->NewByteArray(static_cast<jsize>(dig.data_size()));

      if (env->ExceptionCheck() || ret == nullptr) {
         return nullptr;
      }
      env->SetByteArrayRegion(ret, 0, static_cast<jsize>(dig.data_size()), reinterpret_cast<jbyte*>(dig.data()));

      if (env->ExceptionCheck()) {
         env->ReleaseByteArrayElements(ret, reinterpret_cast<jbyte*>(dig.data()), 0);
         return nullptr;
      }

      return ret;
   } catch (const fc::exception& e) {
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      return nullptr;
   }
}

JNIEXPORT jbyteArray JNICALL Java_AlexandriaJNI_signDigest(JNIEnv *env, jclass, jbyteArray inJNIDigest,
                                                           jbyteArray inJNIPrivateKey) {

   int length = env->GetArrayLength(inJNIDigest);
   if(length == -1) {
      return nullptr;
   }

   std::vector<char> digest((size_t)length);
   env->GetByteArrayRegion(inJNIDigest, 0, length, reinterpret_cast<jbyte*>(&digest.front()));

   if (env->ExceptionCheck()) {
      return nullptr;
   }

   length = env->GetArrayLength(inJNIPrivateKey);
   if(length == -1) {
      return nullptr;
   }

   std::vector<char> private_key((size_t)length);
   env->GetByteArrayRegion(inJNIPrivateKey, 0, length, reinterpret_cast<jbyte*>(&private_key.front()));

   if (env->ExceptionCheck()) {
      return nullptr;
   }

    try {
       fc::sha256 dig(digest.data(), digest.size());
       fc::ecc::private_key key = fc::variant(private_key).as<fc::ecc::private_key>();
       auto sig = key.sign_compact(dig, fc::ecc::bip_0062);

       jbyteArray ret = env->NewByteArray(static_cast<jsize>(sig.size()));

       if (env->ExceptionCheck() || ret == nullptr) {
          return nullptr;
       }
       env->SetByteArrayRegion(ret, 0, static_cast<jsize>(sig.size()), reinterpret_cast<jbyte*>(sig.data));

       if (env->ExceptionCheck()) {
          env->ReleaseByteArrayElements(ret, reinterpret_cast<jbyte*>(sig.data), 0);
          return nullptr;
       }

       return ret;
    } catch (const fc::exception& e) {

       return nullptr;
    }
 }

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_addSignature(JNIEnv *env, jclass, jstring inJNIStrTransaction,
                                                          jbyteArray inJNISignature) {

   const char *transaction = env->GetStringUTFChars(inJNIStrTransaction, 0);
   if (transaction == nullptr) {
      return nullptr;
   }

   int length = env->GetArrayLength(inJNISignature);
   if(length != 65) {
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      return nullptr;
   }

   compact_signature signature;
   env->GetByteArrayRegion(inJNISignature, 0, 65, reinterpret_cast<jbyte*>(&signature));

   if (env->ExceptionCheck()) {
      env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
      return nullptr;
   }

    try {
       string tx_str(transaction);
       fc::variant v = fc::json::from_string( tx_str, fc::json::strict_parser );
       signed_transaction stx;
       fc::from_variant( v, stx );

       stx.signatures.push_back(signature);

       env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
       return env->NewStringUTF(fc::json::to_string(stx).c_str());

    } catch (const fc::exception& e) {
       env->ReleaseStringUTFChars(inJNIStrTransaction, transaction);
       return nullptr;
    }
 }

JNIEXPORT jboolean JNICALL Java_AlexandriaJNI_verifySignature(JNIEnv *env, jclass, jbyteArray inJNIDigest,
                                                              jbyteArray inJNIPublicKey, jbyteArray inJNISignature) {
   int length = env->GetArrayLength(inJNISignature);
   if(length != 65) {
      return static_cast<jboolean>(false);
   }

   compact_signature signature;
   env->GetByteArrayRegion(inJNISignature, 0, 65, reinterpret_cast<jbyte*>(&signature));

   if (env->ExceptionCheck()) {
      return static_cast<jboolean>(false);
   }

   length = env->GetArrayLength(inJNIDigest);
   if(length == -1) {
      return static_cast<jboolean>(false);
   }

   std::vector<char> digest((size_t)length);
   env->GetByteArrayRegion(inJNIDigest, 0, length, reinterpret_cast<jbyte*>(&digest.front()));

   if (env->ExceptionCheck()) {
      return static_cast<jboolean>(false);
   }

   length = env->GetArrayLength(inJNIPublicKey);
   if(length == -1) {
      return static_cast<jboolean>(false);
   }

   std::vector<char> pub_key((size_t)length);
   env->GetByteArrayRegion(inJNIPublicKey, 0, length, reinterpret_cast<jbyte*>(&pub_key.front()));

   if (env->ExceptionCheck()) {
      return static_cast<jboolean>(false);
   }

   try {
       fc::sha256 dig(digest.data(), digest.size());

       auto bin_key = fc::raw::unpack_from_vector<public_key_type::binary_key>(pub_key, 0);
       public_key_type public_key(bin_key.data);

       if(public_key == fc::ecc::public_key::recover_key(signature, dig, fc::ecc::bip_0062)) {
          return static_cast<jboolean>(true);
       }
   } catch (const fc::exception& e) {
      return static_cast<jboolean>(false);
   }
   return static_cast<jboolean>(false);
 }

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_encryptMemo(JNIEnv *env, jclass, jstring inJNIStrMemo,
                                                         jbyteArray inJNIPrivateKey, jbyteArray inJNIPublicKey) {

   const char *memo = env->GetStringUTFChars(inJNIStrMemo, 0);
   if (memo == nullptr) {
      return nullptr;
   }

   int length = env->GetArrayLength(inJNIPrivateKey);
   if(length == -1) {
      return nullptr;
   }

   std::vector<char> private_key((size_t)length);
   env->GetByteArrayRegion(inJNIPrivateKey, 0, length, reinterpret_cast<jbyte*>(&private_key.front()));

   if (env->ExceptionCheck()) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      return nullptr;
   }

   length = env->GetArrayLength(inJNIPublicKey);
   if(length == -1) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      return nullptr;
   }

   std::vector<char> pub_key((size_t)length);
   env->GetByteArrayRegion(inJNIPublicKey, 0, length, reinterpret_cast<jbyte*>(&pub_key.front()));

   if (env->ExceptionCheck()) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      return nullptr;
   }

    try {
       Java_AlexandriaJNI_memo_data m;

       auto bin_key = fc::raw::unpack_from_vector<public_key_type::binary_key>(pub_key, 0);
       public_key_type public_key(bin_key.data);

       fc::ecc::private_key key = fc::variant(private_key).as<fc::ecc::private_key>();

       m.nonce = fc::time_point::now().time_since_epoch().count();

       auto shared_secret = key.get_shared_secret( public_key );

       fc::sha512::encoder enc;
       fc::raw::pack( enc, m.nonce );
       fc::raw::pack( enc, shared_secret );
       auto encrypt_key = enc.result();

       m.encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack_to_vector(string(memo)) );
       m.check = fc::sha256::hash( encrypt_key )._hash[0];

       env->ReleaseStringUTFChars(inJNIStrMemo, memo);

       return env->NewStringUTF(string(m).c_str());

    } catch (const fc::exception& e) {
       env->ReleaseStringUTFChars(inJNIStrMemo, memo);
       return nullptr;
    }
 }

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_decryptedMemo(JNIEnv *env, jclass, jstring inJNIStrMemo,
                                                           jbyteArray inJNIPrivateKey, jbyteArray inJNIPublicKey) {

   const char *memo = env->GetStringUTFChars(inJNIStrMemo, 0);
   if (memo == nullptr) {
      return nullptr;
   }

   int length = env->GetArrayLength(inJNIPrivateKey);
   if(length == -1) {
      return nullptr;
   }

   std::vector<char> private_key((size_t)length);
   env->GetByteArrayRegion(inJNIPrivateKey, 0, length, reinterpret_cast<jbyte*>(&private_key.front()));

   if (env->ExceptionCheck()) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      return nullptr;
   }

   length = env->GetArrayLength(inJNIPublicKey);
   if(length == -1) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      return nullptr;
   }

   std::vector<char> pub_key((size_t)length);
   env->GetByteArrayRegion(inJNIPublicKey, 0, length, reinterpret_cast<jbyte*>(&pub_key.front()));

   if (env->ExceptionCheck()) {
      env->ReleaseStringUTFChars(inJNIStrMemo, memo);
      return nullptr;
   }

    try {
       string str_memo(memo);
       auto m = Java_AlexandriaJNI_memo_data::from_string( str_memo );

       if( m ) {
          fc::sha512 shared_secret;
          auto bin_key = fc::raw::unpack_from_vector<public_key_type::binary_key>(pub_key, 0);
          public_key_type public_key(bin_key.data);

          fc::ecc::private_key key = fc::variant(private_key).as<fc::ecc::private_key>();

          shared_secret = key.get_shared_secret(public_key);

          fc::sha512::encoder enc;
          fc::raw::pack( enc, m->nonce );
          fc::raw::pack( enc, shared_secret );
          auto encryption_key = enc.result();

          uint64_t check = fc::sha256::hash( encryption_key )._hash[0];
          if( check != m->check ) {
             env->ReleaseStringUTFChars(inJNIStrMemo, memo);
             return nullptr;
          }

          vector<char> decrypted = fc::aes_decrypt( encryption_key, m->encrypted );

          env->ReleaseStringUTFChars(inJNIStrMemo, memo);
          return env->NewStringUTF(fc::raw::unpack_from_vector<std::string>( decrypted, 0 ).c_str());
       } else {
          env->ReleaseStringUTFChars(inJNIStrMemo, memo);
          return nullptr;
       }
    } catch (const fc::exception& e) {
       env->ReleaseStringUTFChars(inJNIStrMemo, memo);
       return nullptr;
    }
}

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_publicKeyToString(JNIEnv *env, jclass, jbyteArray inJNIPublicKey) {

   int length = env->GetArrayLength(inJNIPublicKey);
      if(length == -1) {
      return nullptr;
   }

   std::vector<char> pub_key((size_t)length);
   env->GetByteArrayRegion(inJNIPublicKey, 0, length, reinterpret_cast<jbyte*>(&pub_key.front()));

   if (env->ExceptionCheck()) {
      return nullptr;
   }
   try {
      auto bin_key = fc::raw::unpack_from_vector<public_key_type::binary_key>(pub_key, 0);
      public_key_type public_key(bin_key.data);
      auto public_key_str = fc::json::to_string(public_key);
      return env->NewStringUTF(public_key_str.substr(1, public_key_str.size() - 2).c_str());

   } catch (const fc::exception& e) {
      return nullptr;
   }

}

JNIEXPORT jbyteArray JNICALL Java_AlexandriaJNI_stringToPublicKey(JNIEnv *env, jclass, jstring inJNIStrPublicKey) {

   const char *public_key = env->GetStringUTFChars(inJNIStrPublicKey, 0);
   if (public_key == nullptr) {
      return nullptr;
   }

   try {
      fc::variant v = fc::json::from_string( string(public_key), fc::json::relaxed_parser );
      public_key_type pub_key;
      fc::from_variant( v, pub_key );

      env->ReleaseStringUTFChars(inJNIStrPublicKey, public_key);

      public_key_type::binary_key k;
      k.data = pub_key.key_data;
      k.check = fc::ripemd160::hash( k.data.data, static_cast<uint32_t>(k.data.size()) )._hash[0];
      auto data = fc::raw::pack_to_vector( k );

      jbyteArray ret = env->NewByteArray(static_cast<jsize>(data.size()));

      if (env->ExceptionCheck() || ret == nullptr) {
         return nullptr;
      }
      env->SetByteArrayRegion(ret, 0, static_cast<jsize>(data.size()), reinterpret_cast<jbyte*>(data.data()));

      if (env->ExceptionCheck()) {
         env->ReleaseByteArrayElements(ret, reinterpret_cast<jbyte*>(data.data()), 0);
         return nullptr;
      }

      return ret;
   } catch (const fc::exception& e) {
      return nullptr;
   }

}

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_privateKeyToWif(JNIEnv *env, jclass, jbyteArray array) {
   try {
      int length = env->GetArrayLength(array);
      if(length == -1) {
         return nullptr;
      }

      std::vector<char> private_k((size_t)length);
      env->GetByteArrayRegion(array, 0, length, reinterpret_cast<jbyte*>(&private_k.front()));

      if (env->ExceptionCheck()) {
         return nullptr;
      }

      fc::ecc::private_key key = fc::variant(private_k).as<fc::ecc::private_key>();

      return env->NewStringUTF(key_to_wif(key).c_str());

   } catch (const fc::exception& e) {
      return nullptr;
   }
}


JNIEXPORT jbyteArray JNICALL Java_AlexandriaJNI_wifToPrivateKey(JNIEnv *env, jclass, jstring inJNIStrPrivateKey) {
   const char *private_key = env->GetStringUTFChars(inJNIStrPrivateKey, 0);

   if (private_key == nullptr || env->ExceptionCheck()) {
      return nullptr;
   }

   try {
      auto priv_key = sophiatx::utilities::wif_to_key(string(private_key));
      env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);

      if(!priv_key) {
         return nullptr;
      }

      std::vector<char> key_bytes = fc::variant(*priv_key).as<std::vector<char>>();

      jbyteArray ret = env->NewByteArray(static_cast<jsize>(key_bytes.size()));

      if (env->ExceptionCheck() || ret == nullptr) {
         return nullptr;
      }
      env->SetByteArrayRegion(ret, 0, static_cast<jsize>(key_bytes.size()), reinterpret_cast<jbyte*>(key_bytes.data()));

      if (env->ExceptionCheck()) {
         env->ReleaseByteArrayElements(ret, reinterpret_cast<jbyte*>(key_bytes.data()), 0);
         return nullptr;
      }
      return ret;
   } catch (const fc::exception& e) {
      env->ReleaseStringUTFChars(inJNIStrPrivateKey, private_key);
      return nullptr;
   }
}

JNIEXPORT jstring JNICALL Java_AlexandriaJNI_toBase58(JNIEnv *env, jclass, jbyteArray array) {
   try {
      int length = env->GetArrayLength(array);
      if(length == -1) {
         return nullptr;
      }

      std::vector<char> data((size_t)length);
      env->GetByteArrayRegion(array, 0, length, reinterpret_cast<jbyte*>(&data.front()));

      if (env->ExceptionCheck()) {
         return nullptr;
      }

      return env->NewStringUTF(fc::to_base58(data).c_str());
   } catch (const fc::exception& e) {
      return nullptr;
   }
}

JNIEXPORT jbyteArray JNICALL Java_AlexandriaJNI_fromBase58(JNIEnv *env, jclass, jstring inJNIStrdata) {
   const char *str_data = env->GetStringUTFChars(inJNIStrdata, 0);

   if (str_data == nullptr || env->ExceptionCheck()) {
      return nullptr;
   }

   try {
      std::vector<char> bytes = fc::from_base58(string(str_data));
      env->ReleaseStringUTFChars(inJNIStrdata, str_data);

      jbyteArray ret = env->NewByteArray(static_cast<jsize>(bytes.size()));

      if (env->ExceptionCheck() || ret == nullptr) {
         return nullptr;
      }
      env->SetByteArrayRegion(ret, 0, static_cast<jsize>(bytes.size()), reinterpret_cast<jbyte*>(bytes.data()));

      if (env->ExceptionCheck()) {
         env->ReleaseByteArrayElements(ret, reinterpret_cast<jbyte*>(bytes.data()), 0);
         return nullptr;
      }
      return ret;

   } catch (const fc::exception& e) {
      env->ReleaseStringUTFChars(inJNIStrdata, str_data);
      return nullptr;
   }
}

FC_REFLECT( Java_AlexandriaJNI_memo_data, (nonce)(check)(encrypted) )
