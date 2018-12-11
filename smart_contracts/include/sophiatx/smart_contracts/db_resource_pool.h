#ifndef SOPHIATX_DB_RESOURCE_POOL_H
#define SOPHIATX_DB_RESOURCE_POOL_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <sophiatx/smart_contracts/db_resource.h>

namespace sophiatx { namespace smart_contracts {


/**
 * @brief resource error exception
 */
class resource_error : public std::runtime_error {
public:
   /**
    * @brief create resource error of given type
    * @param type
    */
   explicit resource_error(const std::string& msg = "") : std::runtime_error( getMessage(msg) ) {}
   virtual ~resource_error() = default;

private:
   static std::string getMessage(const std::string& msg) {
      return "Unable to get db resource. Error: " + msg;
   }
};



/**
 * @brief Synchronous database resource pool. It stores maximum <max_resources> opened db connections(resources), which are mapped to the account names.
 *        In case we need more connections, the least used one is deleted and replaced with the one, which is needed at the moment.
 */
class db_resource_pool {
public:
   db_resource_pool(uint32_t max_handles_count = 500 /*TODO: read from config*/);

   /**
    * @brief Creates new resource mapped to the account_name. In case there is already <max_resources> resources created, it deletes(from memory) the least used one.
    *
    * @throws sophiatx::smart_contracts::resource_error in case creation was not successful
    * @param account_name
    * @return created SQLite::Database&
    */
   SQLite::Database& create_resource(const std::string &account_name);

   /**
    * @brief Returns database resource according to provided account_name(smart_contract acc). In case such resource does not exist, it creates one.
    *
    * @throws sophiatx::smart_contracts::resource_error in case resource does not exist(and create_flag==false) or there was error during processing
    * @param account_name
    * @param create_flag if set to true, it will create resource in case it does not exist
    * @return SQLite::Database&
    */
   SQLite::Database& get_resource(const std::string &account_name, bool create_flag = true);

   /**
    * @return actual number of resources in the pool
    */
   const uint32_t size() const;
private:
   /**
    * @brief Defines boost multiindex container that stores db resource objects
    */
   struct by_account_name;
   struct by_last_access;
   using db_resources_index =
   boost::multi_index::multi_index_container<
         db_resource,
         boost::multi_index::indexed_by<
               boost::multi_index::ordered_unique< boost::multi_index::tag< by_account_name >, boost::multi_index::member< db_resource, std::string, &db_resource::account_name > >,
               boost::multi_index::ordered_non_unique< boost::multi_index::tag< by_last_access >, boost::multi_index::member < db_resource, std::chrono::system_clock::time_point, &db_resource::last_access > >
         >
   >;
   using db_resources_by_name_index = db_resources_index::index<by_account_name>::type;
   using db_resources_by_last_access_index = db_resources_index::index<by_last_access>::type;


   /**
    * @brief Pops the least used resource
    *
    */
   void pop_resource();



   db_resources_index                    db_resources;
   db_resources_by_name_index&           db_resources_by_name;
   db_resources_by_last_access_index&    db_resources_by_last_access;

   // maximum number of opened db resources/handles
   uint32_t                              max_resources;
};


}}

#endif //SOPHIATX_DB_RESOURCE_POOL_H

