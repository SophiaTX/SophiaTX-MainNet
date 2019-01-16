#pragma once

#include <sophiatx/chain/database/database_interface.hpp>

namespace sophiatx {
namespace chain {

template<typename MultiIndexType>
void _add_index_impl(const std::weak_ptr<database_interface> &db) {
   if( auto ptr = db.lock() ) {
      ptr->add_index<MultiIndexType>();
   } else {
      FC_ASSERT(false, "DB pointer does not exist!");
   }
}

template<typename MultiIndexType>
void add_core_index(const std::weak_ptr<database_interface> &db) {
   _add_index_impl<MultiIndexType>(db);
}

template<typename MultiIndexType>
void add_plugin_index(const std::shared_ptr<database_interface> &db) {
   db->_plugin_index_signal.connect([ wp = std::weak_ptr<database_interface>(db) ]() { _add_index_impl<MultiIndexType>(wp); });
}

}
}
