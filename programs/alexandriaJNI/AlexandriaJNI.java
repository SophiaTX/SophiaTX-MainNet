public class AlexandriaJNI {
   static {
      System.loadLibrary("alexandriaJNI"); // Load native library at runtime
   }
 
   public static native byte[] generatePrivateKey();
   public static native byte[] generatePrivateKeyFromBrainKey(String brain_key);
   public static native byte[] getPublicKey(byte[] private_key);
   public static native byte[] getTransactionDigest(String transaction, byte[] chain_id);
   public static native byte[] signDigest(byte[] digest, byte[] private_key);
   public static native String addSignature(String transaction, byte[] signature);
   public static native boolean verifySignature(byte[] digest, byte[] public_key, byte[] signed_digest);
   public static native String encryptMemo(String memo, byte[] private_key, byte[] public_key);
   public static native String decryptedMemo(String memo, byte[] private_key, byte[] public_key);
   public static native String privateKeyToWif(byte[] private_key);
   public static native byte[] wifToPrivateKey(String wif_key);
   public static native String toBase58(byte[] data);
   public static native byte[] fromBase58(String data);


   // Test Driver
   public static void main(String[] args) {
         // String tx = "{\"id\":0,\"result\":{\"ref_block_num\":28367,\"ref_block_prefix\":785299212,\"expiration\":\"2018-07-04T11:19:03\",\"operations\":[[\"account_create\",{\"fee\":\"0.100000 SPHTX\",\"creator\":\"GN3Ug8ou6tiE4kWvyopJBJmZgcw\",\"name_seed\":\"sanjiv\",\"owner\":{\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",1]]},\"active\":{\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",1]]},\"memo_key\":\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",\"json_metadata\":\"{}\"}]],\"extensions\":[],\"signatures\":[]}}";
         // AlexandriaJNI test = new AlexandriaJNI();
         // String[] key_pair = test.generateKeyPair();
         // // System.out.println(key_pair[0]);
         // // System.out.println(test.getPublicKey(key_pair[1]));
         // if(!test.getPublicKey(key_pair[1]).equals(key_pair[0]))
         // {
         //     System.out.println("getPublicKey failed");
         // }
         // String digest = test.getTransactionDigest(tx, "0000000000000000000000000000000000000000000000000000000000000000");
         // String signature = test.signDigest(digest, key_pair[1]);
         // if(!test.verifySignature(digest, key_pair[0], signature))
         // {
         //     System.out.println("Transaction signing failed");
         // }

         // String[] key_pairB = test.generateKeyPair();
         // String memo = "testabc";
         // String enctryped = test.encryptMemo(memo, key_pair[1], key_pairB[0]);
         // if(!test.decryptedMemo(enctryped, key_pairB[1], key_pair[0]).equals(memo))
         // {
         //    System.out.println("Encryption failed");
         // }
   }
}