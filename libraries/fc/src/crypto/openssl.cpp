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
       static ENGINE* eng;

       openssl_scope()
       {
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

    ENGINE* openssl_scope::eng;

    int init_openssl()
    {
      static openssl_scope ossl;
      return 0;
    }
}
