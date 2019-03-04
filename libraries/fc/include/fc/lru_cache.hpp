#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/optional.hpp>
#include <chrono>
#include <mutex>

namespace fc {

/**
 * @brief custom LruCache exception type
 */
class LruCacheError : public std::runtime_error {
public:
   enum class Type {
      TIMEOUT,
      OTHER
   };

   explicit LruCacheError(Type type, const std::string& msg = "") :
         std::runtime_error(getMessage(type, msg)),
         type_(type)
   {}

   const Type type() const { return type_; }
   virtual ~LruCacheError() throw() {}

private:
   Type type_;

   /**
    * @brief get exception what() message for given resource type
    * @param type
    * @return exception what() message
    */
   static std::string getMessage(const Type type, const std::string& msg) {
      switch (type) {
         case Type::TIMEOUT: return "LruCache Error: TIMEOUT";
         case Type::OTHER:   return "LruCache Error: OTHER, msg: " + msg;
         default:            return "LruCache Error: UNKNOWN";
      }
   }
};


/**
 * @brief Thread-safe timed Least-recently-used resource cache. It stores maximum <max_resources_> resources of type "ValueType", which are mapped to the keys of type "KeyType".
 *        In case max. num of resources is reached, the least used one is deleted and replaced with the one, which is needed at the moment. If ResourceCreator(lambda
 *        function) is provided, it is used for automatic resource creations, otherwise provided parameter in emplace method are forwarded to the ValueType constructor
 *
 * @tparam KeyType
 * @tparam ValueType
 * @tparam ResourceCreator
 */
template<typename KeyType, typename ValueType, typename ResourceCreator = void*>
class LruCache {
public:
   static const std::chrono::milliseconds default_timeout;
   static const std::chrono::milliseconds default_update_interval;

   /**
    * @brief Ctor
    *
    * @param max_resources_count max number of resources in cache. In case there is need to create new resource, least used one is automacially deleted.
    * @param timeout maximum time that is waited until resource is emplaced/get. This waiting time might be too long in case too many concurrent threads
    *           are trying to emplaced/get resources so they block each other. In such case, LruCache::Error with interal type(TIMEOUT) is thrown
    * @param update_interval min. interval [ms], after which is access_time updated when trying to obtain resource.
    *           If now() - resource.last_access_time < update_interval, resource.last_access_time is not updated. It should save a lot of useless locks
    *           that might happen if we get one resource multiple times in short period
    *
    * @param resource_creator lambda that creates resource of type "ValueType"
    */
   template<typename T = ResourceCreator, typename = typename std::enable_if<!std::is_same<void*, T>::value>::type>
   LruCache(uint32_t max_resources_count,
            const ResourceCreator& resource_creator,
            const std::chrono::milliseconds& timeout = default_timeout,
            const std::chrono::milliseconds& update_interval = default_update_interval) :
      resource_creator_(resource_creator),
      mutex_(),
      timeout_(timeout),
      update_access_interval_(update_interval),
      max_resources_(max_resources_count),
      resources_(),
      resources_by_key_(resources_.template get<by_key>()),
      resources_by_last_access_(resources_.template get<by_last_access>())
   {}

   /**
    * @brief Ctor
    *
    * @param max_resources_count max number of resources in cache. In case there is need to create new resource, least used one is automacially deleted.
    * @param timeout maximum time that is waited until resource is emplaced/get. This waiting time might be too long in case too many concurrent threads
    *           are trying to emplaced/get resources so they block each other. In such case, LruCache::Error with interal type(TIMEOUT) is thrown
    * @param update_interval min. interval [ms], after which is access_time updated when trying to obtain resource.
    *           If now() - resource.last_access_time < update_interval, resource.last_access_time is not updated. It should save a lot of useless locks
    *           that might happen if we get one resource multiple times in short period
    */
   template<typename T = ResourceCreator, typename = typename std::enable_if<std::is_same<void*, T>::value>::type>
   LruCache(uint32_t max_resources_count,
            const std::chrono::milliseconds& timeout = default_timeout,
            const std::chrono::milliseconds& update_interval = default_update_interval) :
         resource_creator_(nullptr),
         mutex_(),
         timeout_(timeout),
         update_access_interval_(update_interval),
         max_resources_(max_resources_count),
         resources_(),
         resources_by_key_(resources_.template get<by_key>()),
         resources_by_last_access_(resources_.template get<by_last_access>())
   {}

   // Disables default/copy/move ctors
   LruCache()                            = delete;
   LruCache(const LruCache &)            = delete;
   LruCache &operator=(const LruCache &) = delete;
   LruCache(LruCache &&)                 = delete;
   LruCache &operator=(LruCache &&)      = delete;

   /**
    * @brief Creates new resource mapped to the key(only if it does not exist in the cache already). For creation is
    *        used forwarding to the ValueType constructor
    *
    * @tparam ArgsType
    * @param key that resource is mapped to
    * @param args arguments forwarded to the resource_creator_
    * @throws LruCacheError in case of failure
    *
    * @return ValueType& - reference to the resource of mapped to the key
    */
   template <typename T = ResourceCreator, typename... ArgsType>
   typename std::enable_if<std::is_same<T, void*>::value, ValueType&>::type emplace(const KeyType& key, ArgsType&&... args) {
      typename resources_by_key_index::iterator resource = resources_by_key_.find(key);
      if (resource != resources_by_key_.end()) {
         // Adjusts last_access of the found resource
         updateAccessTime(resource);

         return resource->getValue();
      }

      std::unique_lock<std::timed_mutex> lock(mutex_, timeout_);
      checkLock(lock);

      if (resources_by_key_.size() >= max_resources_) {
         popLru();
      }

      // Creates new resource inside cache
      auto emplace_result = resources_by_key_.emplace(key, std::forward<ArgsType>(args)...);
      if (emplace_result.second == false) {
         throw LruCacheError(LruCacheError::Type::OTHER, "Unable to create resource");
      }

      return emplace_result.first->getValue();
   }


   /**
    * @brief Creates new resource mapped to the key(only if it does not exist in the cache already). For creation is
    *        used lambda function resource_creator_
    *
    * @tparam ArgsType
    * @param key that resource is mapped to
    * @param args arguments forwarded to the resource_creator_
    * @throws LruCacheError in case of failure
    *
    * @return ValueType& - reference to the resource of mapped to the key
    */
   template <typename T = ResourceCreator, typename... ArgsType>
   typename std::enable_if<!std::is_same<T, void*>::value, ValueType&>::type emplace(const KeyType& key, ArgsType&&... args) {
      typename resources_by_key_index::iterator resource = resources_by_key_.find(key);
      if (resource != resources_by_key_.end()) {
         // Adjusts last_access of the found resource
         updateAccessTime(resource);

         return resource->getValue();
      }

      std::unique_lock<std::timed_mutex> lock(mutex_, timeout_);
      checkLock(lock);

      if (resources_by_key_.size() >= max_resources_) {
         popLru();
      }

      // Creates new resource inside cache
      auto emplace_result = resources_by_key_.emplace(key, resource_creator_(std::forward<ArgsType>(args)...));
      if (emplace_result.second == false) {
         throw LruCacheError(LruCacheError::Type::OTHER, "Unable to create resource");
      }

      return emplace_result.first->getValue();
   }


   /**
    * @brief Returns resource(boost::optional<ValueType&>) mapped to the provided key. In case no such resource exist, empty boost::optional is returned. Otherwise
    *        it returnes optional with reference to the resource
    *
    * @param key
    * @throws LruCacheError in case of failure
    *
    * @return boost::optional<ValueType&>
    */
   boost::optional<ValueType&> get(const KeyType& key) {
      boost::optional<ValueType&> ret;

      auto resource = resources_by_key_.find(key);
      if (resource == resources_by_key_.end()) {
         return ret;
      }

      // Adjusts last_access of the found resource
      updateAccessTime(resource);

      ret = resource->getValue();
      return ret;
   }

   /**
    * @return actual number of resources in the cache
    */
   const uint32_t size() const {
      return resources_.size();
   }

   /**
    * @return max size of resources in the cache
    */
   const uint32_t getMaxSize() {
      return max_resources_;
   }

   /**
    * @brief Changes maximum cache size
    * @param max_resources_count max number of resources in cache
    *
    * @throws LruCacheError in case of failure
    */
   void setMaxSize(uint32_t max_resources_count) {
      if(resources_.size() > max_resources_count) {
         throw LruCacheError(LruCacheError::Type::OTHER, "Can not resize cache, because actual size of cache is bigger then new maximum size!");
      }

      std::unique_lock<std::timed_mutex> lock(mutex_, timeout_);
      checkLock(lock);
      max_resources_ = max_resources_count;
   }

   /**
    * @brief Erases resource mapped to the key
    *
    * @param key
    */
   void erase(const KeyType& key) {
      auto resource = resources_by_key_.find(key);
      if (resource == resources_by_key_.end()) {
         return;
      }

      std::unique_lock<std::timed_mutex> lock(mutex_, timeout_);
      checkLock(lock);

      resources_by_key_.erase(resource);
   }

private:
   class Resource {
   public:
      /**
       * Class members are public because it is necessary for usage in boost::multi_index_container.
       */
      KeyType                                key_;
      mutable ValueType                      value_;
      std::chrono::steady_clock::time_point  last_access_;

      template <typename... ArgsType>
      Resource(const KeyType& key, ArgsType&&... args) :
            key_(key),
            value_(std::forward<ArgsType>(args)...),
            last_access_(std::chrono::steady_clock::now())
      {}

      ~Resource() = default;

      // Disables default/copy/move ctors - resources should not be copied
      Resource()                            = delete;
      Resource(const Resource &)            = delete;
      Resource &operator=(const Resource &) = delete;
      Resource(Resource &&)                 = delete;
      Resource &operator=(Resource &&)      = delete;

      void updateAccessTime() { last_access_ = std::chrono::steady_clock::now(); }

      /*
       * getValue() always returns ValueType& and not const ValueType&. This value can be changed. Only values that are multiindex container
       * keys should never be changed
       */
      ValueType& getValue()         { return value_; }
      ValueType& getValue() const   { return value_; }
   };

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
               boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_last_access>, boost::multi_index::member<Resource, std::chrono::steady_clock::time_point, &Resource::last_access_> >
         >
   >;
   using resources_by_key_index           = typename resources_index::template index<by_key>::type;
   using resources_by_last_access_index   = typename resources_index::template index<by_last_access>::type;


   /**
    * @throws LruCacheError in case provided unique_lock has not an associated mutex or has not acquired ownership of it
    */
   void checkLock(const std::unique_lock<std::timed_mutex>& lock) {
      if (!lock) {
         throw LruCacheError(LruCacheError::Type::TIMEOUT);
      }
   }

   /**
    * @brief Updates resource.last_access_ in case: now() - resource.last_access_ >= update_interval,
    *        otherwise it keeps original resource.last_access_ value.
    */
   void updateAccessTime(const typename resources_by_key_index::iterator& it) {
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - it->last_access_);
      if (duration < update_access_interval_) {
         return;
      }

      std::unique_lock<std::timed_mutex> lock(mutex_, timeout_);
      checkLock(lock);

      if (resources_by_key_.modify(it, [](Resource &resource) { resource.updateAccessTime(); }) == false) {
         throw LruCacheError(LruCacheError::Type::OTHER, "Unable to modify last access time of resource");
      }
   }

   /**
    * @brief Pops the least recent used used resource
    */
   void popLru() {
      // Do not lock internal mutex as it is already locked by methods that call this method
      if (resources_by_last_access_.empty() == false) {
         resources_by_last_access_.erase(resources_by_last_access_.begin());
      }
   }


   // Lambda that automatically creates new resource of type ValueType
   ResourceCreator                  resource_creator_;

   // Synchronization attributes
   std::timed_mutex                 mutex_;
   std::chrono::milliseconds        timeout_;
   const std::chrono::milliseconds  update_access_interval_;

   // maximum number of resources
   uint32_t                         max_resources_;
   resources_index                  resources_;
   resources_by_key_index&          resources_by_key_;
   resources_by_last_access_index&  resources_by_last_access_;
};

template<typename KeyType, typename ValueType, typename ResourceCreator>
constexpr std::chrono::milliseconds LruCache<KeyType, ValueType, ResourceCreator>::default_timeout = std::chrono::milliseconds(1000);

template<typename KeyType, typename ValueType, typename ResourceCreator>
constexpr std::chrono::milliseconds LruCache<KeyType, ValueType, ResourceCreator>::default_update_interval = std::chrono::milliseconds(500);

}
#endif //LRU_CACHE_HPP