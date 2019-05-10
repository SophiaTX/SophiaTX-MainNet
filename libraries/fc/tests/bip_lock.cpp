#include <iostream>
#include <fc/interprocess/file_mutex.hpp>
#include <fc/filesystem.hpp>
#include <fc/thread/thread.hpp>

int main( int argc, char** argv ) {
  if( argc < 2 ) return 0;
  fc::file_mutex m( argv[1] );
  auto mptr = &m;

  fc::thread in("in");

  std::string cmd;
  std::cout << ">>> ";
  std::cin >> cmd;
  int i = 0;
  while( !std::cin.eof() && cmd != "q" ) {
    ++i;
    fc::async( [i, cmd,mptr]() {
       std::cout << "start " << cmd << " " << i << std::endl;
       if( cmd == "L" ) {
          mptr->lock();
       } else if( cmd == "l" ) {
          mptr->lock_shared();
       } else if( cmd == "U" ) {
          mptr->unlock();
       } else if( cmd == "u" ) {
          mptr->unlock_shared();
       }
       std::cout << "end " << cmd << " " << i << std::endl;
    } );
    fc::usleep( fc::microseconds( 1000 ) );
    cmd = in.async( [&]() {
       std::string tmp;
       std::cout << m.readers() << std::endl;
       std::cin >> tmp;
       return tmp;
    } );
  }
  std::cout << "done";

  return 0;
}
