#ifndef STEEM_ALEXANDRIA_HPP
#define STEEM_ALEXANDRIA_HPP

extern "C" {

   static bool generate_key_pair(char* private_key);

   static bool get_transaction_digest(const char* transaction, char* digest);

   static bool sign_digest(const char* digest, const char* private_key, char* signed_digest);

   static bool add_signature(const char* transaction, const char* signature, char* signed_tx);
};

#endif //STEEM_ALEXANDRIA_HPP
