#ifndef SOPHIATX_DB_CONNECTIONS_POOL_H
#define SOPHIATX_DB_CONNECTIONS_POOL_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <sophiatx/smart_contracts/db_connection_resource.h>

namespace sophiatx { namespace smart_contracts {


/**
 * @brief Synchronous database connections pool. It stores maximum <max_handles> opened db connections, which are mapped to the account names.
 *        In case we need more connections, the least used one is deleted and replaced with the one, which is needed at the moment.
 */
class db_connections_pool {
public:
   db_connections_pool(uint32_t max_handles_count = 500 /*TODO: read from config*/);

   /**
    * @brief Returns database handle according to provided account_name(smart_contract acc). In case such database does not exist, it creates one.
    *
    * @param account_name
    * @return SQLite::Database&
    */
   SQLite::Database& get_database(const std::string& account_name);
   void test();
private:
   /**
    * @brief Defines boost multiindex container that stores database resource objects
    */
   struct by_account_name;
   struct by_last_access;
   using db_handles_index =
   boost::multi_index::multi_index_container<
         db_connection_resource,
         boost::multi_index::indexed_by<
               boost::multi_index::ordered_unique< boost::multi_index::tag< by_account_name >, boost::multi_index::member< db_connection_resource, std::string, &db_connection_resource::account_name > >,
               boost::multi_index::ordered_non_unique< boost::multi_index::tag< by_last_access >, boost::multi_index::member < db_connection_resource, std::chrono::system_clock::time_point, &db_connection_resource::last_access > >
         >
   >;
   using db_handles_by_name_index = db_handles_index::index<by_account_name>::type;
   using db_handles_by_last_access_index = db_handles_index::index<by_last_access>::type;

   /**
    * @brief Creates new database mapped to the account_name. In case there is already <max_handles> databases creates, it deletes(from memory) the least used one.
    *
    * @param unmapped_account_name
    */
   SQLite::Database& create_database(const std::string& account_name);

   /**
    * @brief Deletes the least used database
    */
   void delete_least_used_database();



   db_handles_index                    db_handles;
   db_handles_by_name_index&           db_handles_by_name;
   db_handles_by_last_access_index&    db_handles_by_last_access;

   // maximum number of opened db handles
   uint32_t                            max_handles;
};


}}

#endif //SOPHIATX_DB_CONNECTIONS_POOL_H
