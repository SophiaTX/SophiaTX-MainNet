#ifndef STEEM_ALEXANDRIA_HPP
#define STEEM_ALEXANDRIA_HPP

extern "C" {

   bool generate_private_key(char* private_key);

   bool get_transaction_digest(const char* transaction, char* digest);

   bool sign_digest(const char* digest, const char* private_key, char* signed_digest);

   bool add_signature(const char* transaction, const char* signature, char* signed_tx);
};

#endif //STEEM_ALEXANDRIA_HPP
