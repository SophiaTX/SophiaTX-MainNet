#ifndef LRU_RESOURCE_POOL_HPP
#define LRU_RESOURCE_POOL_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/optional.hpp>
#include <chrono>

namespace sophiatx { namespace utilities {


/**
 * @brief Non-thread-safe Least-recently-used resource pool. It stores maximum <max_resources_> resources of type "ValueType", which are mapped to the keys of type "KeyType".
 *        In case max. num of resources is reached, the least used one is deleted and replaced with the one, which is needed at the moment. If ResourceCreator(lambda
 *        function) is provided, it is used for automatic resource creations, otherwise provided parameter in emplace method are forwarded to the ValueType constructor
 *
 * @tparam KeyType
 * @tparam ValueType
 * @tparam ResourceCreator
 */
template<typename KeyType, typename ValueType, typename ResourceCreator = void*>
class LruResourcePool {
public:
   /**
    * @brief Ctor
    *
    * @param max_resources_count max number of resources in pool. In case there is need to create new resource, least used one is automacially deleted.
    * @param resource_creator lambda that creates resource of type "ValueType"
    */
   template<typename T = ResourceCreator, typename = typename std::enable_if<!std::is_same<void*, T>::value>::type>
   LruResourcePool(uint32_t max_resources_count, const ResourceCreator& resource_creator) :
      resource_creator_(resource_creator),
      max_resources_(max_resources_count),
      resources_(),
      resources_by_key_(resources_.template get<by_key>()),
      resources_by_last_access_(resources_.template get<by_last_access>())
   {}

   template<typename T = ResourceCreator, typename = typename std::enable_if<std::is_same<void*, T>::value>::type>
   LruResourcePool(uint32_t max_resources_count) :
         resource_creator_(nullptr),
         max_resources_(max_resources_count),
         resources_(),
         resources_by_key_(resources_.template get<by_key>()),
         resources_by_last_access_(resources_.template get<by_last_access>())
   {}


   /**
    * @brief Creates new resource mapped to the key(only if it does not exist in the pool already). For creation is
    *        used forwarding to the ValueType constructor
    *
    * @tparam ArgsType
    * @param key that resource is mapped to
    * @param args arguments forwarded to the resource_creator_
    * @return ValueType& - reference to the resource of mapped to the key
    */
   template <typename T = ResourceCreator, typename... ArgsType>
   typename std::enable_if<std::is_same<T, void*>::value, ValueType&>::type emplace(const KeyType& key, ArgsType&&... args) {
      auto resource = resources_by_key_.find(key);
      if (resource != resources_by_key_.end()) {
         // Adjusts last_access of the found resource
         if (resources_by_key_.modify(resource, [](Resource& resource) { resource.updateAccessTime(); }) == false) {
            throw std::runtime_error("Unable to modify last access time of resource");
         }

         return resource->getValue();
      }

      if (resources_by_key_.size() >= max_resources_) {
         popLru();
      }

      // Creates new database handle inside pool
      auto emplace_result = resources_by_key_.emplace(key, std::forward<ArgsType>(args)...);
      if (emplace_result.second == false) {
         throw std::runtime_error("Unable to create resource");
      }

      return emplace_result.first->getValue();
   }


   /**
    * @brief Creates new resource mapped to the key(only if it does not exist in the pool already). For creation is
    *        used lambda function resource_creator_
    *
    * @tparam ArgsType
    * @param key that resource is mapped to
    * @param args arguments forwarded to the resource_creator_
    * @return ValueType& - reference to the resource of mapped to the key
    */
   template <typename T = ResourceCreator, typename... ArgsType>
   typename std::enable_if<!std::is_same<T, void*>::value, ValueType&>::type emplace(const KeyType& key, ArgsType&&... args) {
      auto resource = resources_by_key_.find(key);
      if (resource != resources_by_key_.end()) {
         // Adjusts last_access of the found resource
         if (resources_by_key_.modify(resource, [](Resource& resource) { resource.updateAccessTime(); }) == false) {
            throw std::runtime_error("Unable to modify last access time of resource");
         }

         return resource->getValue();
      }

      if (resources_by_key_.size() >= max_resources_) {
         popLru();
      }

      // Creates new database handle inside pool
      auto emplace_result = resources_by_key_.emplace(key, resource_creator_(std::forward<ArgsType>(args)...));
      if (emplace_result.second == false) {
         throw std::runtime_error("Unable to create resource");
      }

      return emplace_result.first->getValue();
   }


   /**
    * @brief Returns resource(boost::optional<ValueType&>) mapped to the provided key. In case no such resource exist, empty boost::optional is returned. Otherwise
    *        it returnes optional with reference to the resource
    *
    * @param key
    * @return boost::optional<ValueType&>
    */
   boost::optional<ValueType&> get(const KeyType& key) {
      boost::optional<ValueType&> ret;

      auto resource = resources_by_key_.find(key);
      if (resource == resources_by_key_.end()) {
         return ret;
      }

      // Adjusts last_access of the found resource
      if (resources_by_key_.modify(resource, [](Resource& resource) { resource.updateAccessTime(); }) == false) {
         throw std::runtime_error("Unable to modify last access time of resource");
      }
      

      ret = resource->getValue();
      return ret;
   }

   /**
    * @return actual number of resources in the pool
    */
   const uint32_t size() const {
      return resources_.size();
   }

   /**
    * @brief Pops the least recent used used resource
    */
   void popLru() {
      if (resources_by_last_access_.empty() == false) {
         resources_by_last_access_.erase(resources_by_last_access_.begin());
      }
   }

   /**
    * @brief Erases resource mapped to the key
    *
    * @param key
    */
   void erase(const KeyType& key) {
      auto resource = resources_by_key_.find(key);
      if (resource != resources_by_key_.end()) {
         resources_by_key_.erase(resource);
      }
   }

private:
   class Resource {
   public:
      /**
       * Class members are public because it is necessary for usage in boost::multi_index_container.
       */
      KeyType                                key_;
      mutable ValueType                      value_;
      std::chrono::system_clock::time_point  last_access_;

      template <typename... ArgsType>
      Resource(const KeyType& key, ArgsType&&... args) :
            key_(key),
            value_(std::forward<ArgsType>(args)...),
            last_access_(std::chrono::system_clock::now())
      {}

      ~Resource() = default;

      // Disables default/copy/move ctors - resources should not be copied
      Resource()                            = delete;
      Resource(const Resource &)            = delete;
      Resource &operator=(const Resource &) = delete;
      Resource(Resource &&)                 = delete;
      Resource &operator=(Resource &&)      = delete;

      void updateAccessTime() { last_access_ = std::chrono::system_clock::now(); }

      /*
       * getValue() always returns ValueType& and not const ValueType&. This value can be changed. Only values that are multiindex container
       * keys should never be changed
       */
      ValueType& getValue()         { return value_; }
      ValueType& getValue() const   { return value_; }
   };

   // Lambda that automatically creates new resource of type ValueType
   ResourceCreator resource_creator_;


   /**
    * @brief Defines boost multiindex container that stores resource objects
    */
   struct by_key;
   struct by_last_access;

   using resources_index =
   boost::multi_index::multi_index_container<
         Resource,
         boost::multi_index::indexed_by<
               boost::multi_index::ordered_unique<boost::multi_index::tag<by_key>, boost::multi_index::member<Resource, KeyType, &Resource::key_> >,
               boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_last_access>, boost::multi_index::member<Resource, std::chrono::system_clock::time_point, &Resource::last_access_> >
         >
   >;
   using resources_by_key_index           = typename resources_index::template index<by_key>::type;
   using resources_by_last_access_index   = typename resources_index::template index<by_last_access>::type;

   // maximum number of resources
   uint32_t                         max_resources_;
   resources_index                  resources_;
   resources_by_key_index&          resources_by_key_;
   resources_by_last_access_index&  resources_by_last_access_;
};

}}
#endif //LRU_RESOURCE_POOL_HPP