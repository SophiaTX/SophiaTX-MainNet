#ifndef SOPHIATX_DB_CONNECTIONS_POOL_H
#define SOPHIATX_DB_CONNECTIONS_POOL_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <sophiatx/smart_contracts/db_connection_resource.h>

namespace sophiatx { namespace smart_contracts {


//using namespace boost::multi_index;

struct by_account;
struct by_last_access;

typedef boost::multi_index::multi_index_container<
   db_connection_resource,
      boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique< boost::multi_index::tag< by_account >, boost::multi_index::member< db_connection_resource, std::string, &db_connection_resource::account_name > >
//            boost::multi_index::ordered_non_unique< boost::multi_index::tag< by_last_access >, boost::multi_index::member < possession_object, account_name_type, &possession_object::holder > >,
   >
> db_connections_index;





class db_connections_pool {
public:
   db_connections_pool();

private:

};


}}

#endif //SOPHIATX_DB_CONNECTIONS_POOL_H
