public class AlexandriaJNI {
   static {
      System.loadLibrary("alexandriaJNI"); // Load native library at runtime
   }
 
   public native String[] generateKeyPair();
   public native String[] generateKeyPairFromBrainKey(String brain_key);
   public native String getPublicKey(String private_key);
   public native String getTransactionDigest(String transaction, String chain_id);
   public native String signDigest(String digest, String private_key);
   public native String addSignature(String transaction, String signature);
   public native boolean verifySignature(String digest, String public_key, String signed_digest);
   public native String encryptMemo(String memo, String private_key, String public_key);
   public native String decryptedMemo(String memo, String private_key, String public_key);


   // Test Driver
   public static void main(String[] args) {
      String tx = "{\"id\":0,\"result\":{\"ref_block_num\":28367,\"ref_block_prefix\":785299212,\"expiration\":\"2018-07-04T11:19:03\",\"operations\":[[\"account_create\",{\"fee\":\"0.100000 SPHTX\",\"creator\":\"GN3Ug8ou6tiE4kWvyopJBJmZgcw\",\"name_seed\":\"sanjiv\",\"owner\":{\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",1]]},\"active\":{\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",1]]},\"memo_key\":\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",\"json_metadata\":\"{}\"}]],\"extensions\":[],\"signatures\":[]}}";
      AlexandriaJNI test = new AlexandriaJNI();
      String[] key_pair = test.generateKeyPair();
      // System.out.println(key_pair[0]);
      // System.out.println(test.getPublicKey(key_pair[1]));
      if(!test.getPublicKey(key_pair[1]).equals(key_pair[0]))
      {
          System.out.println("getPublicKey failed");
      }
      String digest = test.getTransactionDigest(tx, "0000000000000000000000000000000000000000000000000000000000000000");
      String signature = test.signDigest(digest, key_pair[1]);
      if(!test.verifySignature(digest, key_pair[0], signature))
      {
          System.out.println("Transaction signing failed");
      }

      String[] key_pairB = test.generateKeyPair();
      String memo = "testabc";
      String enctryped = test.encryptMemo(memo, key_pair[1], key_pairB[0]);
      if(!test.decryptedMemo(enctryped, key_pairB[1], key_pair[0]).equals(memo))
      {
         System.out.println("Encryption failed");
      }
   }
}