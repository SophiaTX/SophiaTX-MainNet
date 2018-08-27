#include <fc/crypto/openssl.hpp>

#include <fc/filesystem.hpp>

#include <boost/filesystem/path.hpp>

#include <cstdlib>
#include <string>
#include <stdlib.h>


namespace  fc 
{
    struct openssl_scope
    {
       static path _configurationFilePath;
       static ENGINE* eng;

       openssl_scope()
       {
          ERR_load_crypto_strings(); 
          OpenSSL_add_all_algorithms();

          const boost::filesystem::path& boostPath = _configurationFilePath;
          if(boostPath.empty() == false)
          {
            std::string varSetting("OPENSSL_CONF=");
            varSetting += _configurationFilePath.to_native_ansi_path();
#if defined(WIN32)
            _putenv((char*)varSetting.c_str());
#else
            putenv((char*)varSetting.c_str());
#endif
          }

          OPENSSL_config(nullptr);

          ENGINE_load_rdrand();
          ENGINE* eng = ENGINE_by_id("rdrand");
          if(NULL == eng) {
             clean_up_engine();
             return;
          }

          if(!ENGINE_init(eng)) {
             clean_up_engine();
             return;
          }

          if(!ENGINE_set_default(eng, ENGINE_METHOD_RAND)) {
             clean_up_engine();
          }
       }

       ~openssl_scope()
       {
          EVP_cleanup();
          ERR_free_strings();
          clean_up_engine();
       }

       void clean_up_engine()
       {
          if(eng != NULL) {
             ENGINE_finish(eng);
             ENGINE_free(eng);
          }
          ENGINE_cleanup();
       }
    };

    path openssl_scope::_configurationFilePath;
    ENGINE* openssl_scope::eng;

    void store_configuration_path(const path& filePath)
    {
      openssl_scope::_configurationFilePath = filePath;
    }
   
    int init_openssl()
    {
      static openssl_scope ossl;
      return 0;
    }
}
