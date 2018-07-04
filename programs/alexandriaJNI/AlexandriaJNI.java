public class AlexandriaJNI {
   static {
      System.loadLibrary("alexandriaJNI"); // Load native library at runtime
                                   // hello.dll (Windows) or libhello.so (Unixes)
   }
 
   private native String[] generateKeyPair();
   private native String[] generateKeyPairFromBrainKey(String brain_key);
   private native String getPublicKey(String private_key);
   private native String getTransactionDigest(String transaction, String chain_id);
   private native String signDigest(String digest, String private_key);
   private native String addSignature(String transaction, String signature);
   private native boolean verifySignature(String digest, String public_key, String signed_digest);
   private native String encryptMemo(String memo, String private_key, String public_key);
   private native String decryptedMemo(String memo, String private_key, String public_key);

 
   // Test Driver
   public static void main(String[] args) {
      AlexandriaJNI test = new AlexandriaJNI();
      System.out.println("getPublicKey output: " + test.getPublicKey("5JPwY3bwFgfsGtxMeLkLqXzUrQDMAsqSyAZDnMBkg7PDDRhQgaV"));
   }
}

