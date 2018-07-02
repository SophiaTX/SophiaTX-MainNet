#ifndef SOPHIATX_ALEXANDRIA_HPP
#define SOPHIATX_ALEXANDRIA_HPP

#ifdef _WIN32
#define ALEXANDRIA_EXPORT __declspec(dllexport)
#else
#define ALEXANDRIA_EXPORT
#endif

namespace {

extern "C" {

   /**
    * Generates private_key in WIF format based on random seed.
    * @param private_key - return parameter of size 51
    * @param public_key - return parameter of size 53
    * @return - true if operation is successful
    */
   ALEXANDRIA_EXPORT bool generate_private_key(char *private_key, char *public_key);

   /**
    * Returns public_key for given private_key
    * @param private_key - Private key in WIF format
    * @param public_key - return paramter public key derived from private_key
    * @return - true if operation is successful
    */
   ALEXANDRIA_EXPORT bool get_public_key(const char *private_key, char *public_key);

   /**
    * Generates new private/public key pair from brian key.​
    * @param brain_key - Brain key that will be used for private/public key generation
    * @param private_key - return parameter of size 51 (WIF)
    * @param public_key - return parameter of size 53
    * @return - true if operation is successful
    */
   ALEXANDRIA_EXPORT bool generate_key_pair_from_brain_key(const char *brain_key, char *private_key, char *public_key);

   /**
    * Creates digest of JSON formatted transaction
    * @param transaction - transaction in JSON format
    * @param chain_id - id of current blockchain
    * @param digest - returned digest of transaction (size 64)
    * @return - true if operation is successful
    */
   ALEXANDRIA_EXPORT bool get_transaction_digest(const char *transaction, const char *chain_id, char *digest);

   /**
    * Creates signature for provided digest
    * @param digest - digest that will be singed
    * @param private_key  - private key for singing in WIF format
    * @param signed_digest - returned signature (size 130)
    * @return - true if operation is successful
    */
    ALEXANDRIA_EXPORT bool sign_digest(const char *digest, const char *private_key, char *signed_digest);

   /**
    * Adds signature to JSON formatted transaction
    * @param transaction - transaction for singing in JSON format
    * @param signature - signature
    * @param signed_tx - returned signed transaction (size variable, depends on size of transaction on input_
    * @return - true if operation is successful
    */
    ALEXANDRIA_EXPORT bool add_signature(const char *transaction, const char *signature, char *signed_tx);

   /**
    * Function for verifying signature base on digest and public key
    * @param digest - digest that will be singed
    * @param public_key - corresponding public key to private_key used fo signing
    * @param signed_digest - digest singed by private_key
    * @return - true if signature is correct
    */
    ALEXANDRIA_EXPORT bool verify_signature(const char *digest, const char *public_key, const char *signed_digest);

   /**
   *  Returns the encrypted memo
   * @param memo - memo that should be encrypted
   * @param private_key - Private key of sender of memo
   * @param public_key - Public key of recipient
   * @param encrypted_memo - return value of encrypted memo
   * @return - true if signature is correct
   */
   ALEXANDRIA_EXPORT bool encrypt_memo(const char *memo, const char *private_key, const char *public_key, char *encrypted_memo);

   /**
    * Returns the decrypted memo if possible given private keys
    * @param memo - memo that should be encrypted
    * @param private_key - Private key of recipient of memo
    * @param public_key - Public key of sender
    * @param decrypted_memo - decrypted memo
    * @return - true if signature is correct
    */
    ALEXANDRIA_EXPORT bool decrypt_memo(const char *memo, const char *private_key, const char *public_key, char *decrypted_memo);
}

#endif //SOPHIATX_ALEXANDRIA_HPP
}
