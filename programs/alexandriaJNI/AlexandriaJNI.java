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
   public static native String publicKeyToString(byte[] publicKey);
   public static native byte[] stringToPublicKey(String publicKey);
   public static native String privateKeyToWif(byte[] private_key);
   public static native byte[] wifToPrivateKey(String wif_key);
   public static native String toBase58(byte[] data);
   public static native byte[] fromBase58(String data);

   public static byte[] hexStringToByteArray(String s) {
    int len = s.length();
    byte[] data = new byte[len / 2];
    for (int i = 0; i < len; i += 2) {
        data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                             + Character.digit(s.charAt(i+1), 16));
    }
    return data;
   }


   // Test Driver
   public static void main(String[] args) {
      String tx = "{\"id\":0,\"result\":{\"ref_block_num\":28367,\"ref_block_prefix\":785299212,\"expiration\":\"2018-07-04T11:19:03\",\"operations\":[[\"account_create\",{\"fee\":\"0.100000 SPHTX\",\"creator\":\"GN3Ug8ou6tiE4kWvyopJBJmZgcw\",\"name_seed\":\"sanjiv\",\"owner\":{\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",1]]},\"active\":{\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",1]]},\"memo_key\":\"SPH6vh1vH3DTzFj2NUpZgpXfNACxUGsXThSpwVLXh9KaYAnJtrUpz\",\"json_metadata\":\"{}\"}]],\"extensions\":[],\"signatures\":[]}}";
      AlexandriaJNI test = new AlexandriaJNI();
      byte[] private_key = test.generatePrivateKey();
      byte[] public_key = test.getPublicKey(private_key);
      System.out.println(test.privateKeyToWif(private_key));
      System.out.println(test.publicKeyToString(public_key));
      byte[] digest = test.getTransactionDigest(tx, test.hexStringToByteArray("0000000000000000000000000000000000000000000000000000000000000000"));
      byte[] signature = test.signDigest(digest, private_key);
      if(!test.verifySignature(digest, public_key, signature))
      {
          System.out.println("Transaction signing failed");
      }

      byte[] private_keyB = test.generatePrivateKey();
      byte[] public_keyB = test.getPublicKey(private_keyB);
      String memo = "testabc";
      String enctryped = test.encryptMemo(memo, private_key, public_keyB);
      if(!test.decryptedMemo(enctryped, private_keyB, public_key).equals(memo))
      {
         System.out.println("Encryption failed");
      }
   }
}
