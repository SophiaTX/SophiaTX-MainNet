
#include <fc/crypto/sha256.hpp>
#include <fc/exception/exception.hpp>

#include <fstream>
#include <iostream>

int main(int argc, char**argv, char** envp)
{
   std::ifstream infile("log_test.txt");
   uint32_t ref_clz;
   std::string str_h;
   uint32_t ref_log;
   uint32_t cases = 0;
   uint32_t errors = 0;

   while( true )
   {
      if( !(infile >> std::hex >> ref_clz) )
         break;
      if( !(infile >> str_h) )
         break;
      if( !(infile >> std::hex >> ref_log) )
         break;
      fc::sha256 h(str_h);
      if( ref_clz != h.clz() )
      {
         std::cerr << "got error on clz(" << str_h << ")" << std::endl;
         ++errors;
      }
      if( ref_log != h.approx_log_32() )
      {
         std::cerr << "got error on log(" << str_h << ")" << std::endl;
         ++errors;
      }
      ++cases;
   }

   std::cerr << "sha256_log_test checked " << cases << " cases, got " << errors << " errors" << std::endl;
   if( errors )
      return 1;
   return 0;
}
