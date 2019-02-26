#pragma once

#include <sophiatx/chain/sophiatx_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <sophiatx/chain/buffer_type.hpp>

namespace sophiatx {
namespace plugins {
namespace custom_tokens {

using namespace std;
using namespace sophiatx::chain;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef SOPHIATX_custom_tokens_SPACE_ID
#define SOPHIATX_custom_tokens_SPACE_ID 122
#endif

enum token_object_types {
   custom_token_object_type = (SOPHIATX_custom_tokens_SPACE_ID << 8),
   custom_token_account_object_type = (SOPHIATX_custom_tokens_SPACE_ID << 8) + 1,
   custom_token_operation_object_type = (SOPHIATX_custom_tokens_SPACE_ID << 8) + 2,
   custom_token_error_object_type = (SOPHIATX_custom_tokens_SPACE_ID << 8) + 3
};

class custom_token_object : public object<custom_token_object_type, custom_token_object> {
public:
   template<typename Constructor, typename Allocator>
   custom_token_object(Constructor &&c, allocator<Allocator> a) {
      c(*this);
   }

   id_type id;

   asset_symbol_type token_symbol;
   account_name_type owner_name;
   uint64_t total_supply;
   uint64_t max_supply = std::numeric_limits<uint64_t>::max();
   uint64_t burned = 0;
   bool paused = false;
};

class custom_token_account_object
      : public object<custom_token_account_object_type, custom_token_account_object> {
public:
   template<typename Constructor, typename Allocator>
   custom_token_account_object(Constructor &&c, allocator<Allocator> a) {
      c(*this);
   }

   id_type id;

   account_name_type account_name;
   asset_symbol_type token_symbol;
   uint64_t amount = 0;

   uint64_t token_sequence = 0;
   uint64_t account_sequence = 0;
};

class custom_token_operation_object
      : public object<custom_token_operation_object_type, custom_token_operation_object> {
public:
   template<typename Constructor, typename Allocator>
   custom_token_operation_object(Constructor &&c, allocator<Allocator> a) : serialized_op(a.get_segment_manager()) {
      c(*this);
   }

   id_type id;

   account_name_type account;
   asset_symbol_type token_symbol;
   uint32_t block = 0;
   buffer_type serialized_op;
   transaction_id_type trx_id;
   fc::time_point_sec timestamp;

   uint64_t token_sequence = 0;
   uint64_t account_sequence = 0;
};

class custom_token_error_object
      : public object<custom_token_error_object_type, custom_token_error_object> {
public:
   template<typename Constructor, typename Allocator>
   custom_token_error_object(Constructor &&c, allocator<Allocator> a) : error(a) {
      c(*this);
   }

   id_type id;
   transaction_id_type trx_id;
   asset_symbol_type token_symbol;
   shared_string error;
   std::chrono::system_clock::time_point time = std::chrono::system_clock::now();

   uint64_t token_sequence = 0;
};

typedef custom_token_object::id_type custom_token_id_type;
typedef custom_token_account_object::id_type custom_token_account_id_type;
typedef custom_token_operation_object::id_type custom_token_transfer_id_type;
typedef custom_token_error_object::id_type custom_token_error_object_id_type;

using namespace boost::multi_index;

struct by_token_symbol;
struct by_token_and_account;
struct by_tx_id;
struct by_time;

typedef multi_index_container<
      custom_token_object,
      indexed_by<
            ordered_unique<tag<by_id>, member<custom_token_object, custom_token_id_type, &custom_token_object::id> >,
            ordered_unique<tag<by_token_symbol>, member<custom_token_object, asset_symbol_type, &custom_token_object::token_symbol> >
      >,
      allocator<custom_token_object>
> token_index;

typedef multi_index_container<
      custom_token_account_object,
      indexed_by<
            ordered_unique<tag<by_id>, member<custom_token_account_object, custom_token_account_id_type, &custom_token_account_object::id> >,
            ordered_non_unique<tag<by_account>, composite_key<custom_token_account_object,
                  member<custom_token_account_object, account_name_type, &custom_token_account_object::account_name>,
                  member<custom_token_account_object, uint64_t, &custom_token_account_object::account_sequence> >,
                  composite_key_compare<std::less<account_name_type>, std::greater<uint64_t> > >,
            ordered_non_unique<tag<by_token_symbol>, composite_key<custom_token_account_object,
                  member<custom_token_account_object, asset_symbol_type, &custom_token_account_object::token_symbol>,
                  member<custom_token_account_object, uint64_t, &custom_token_account_object::token_sequence> >,
                  composite_key_compare<std::less<asset_symbol_type>, std::greater<uint64_t> > >,
            ordered_unique<tag<by_token_and_account>, composite_key<custom_token_account_object,
                  member<custom_token_account_object, asset_symbol_type, &custom_token_account_object::token_symbol>,
                  member<custom_token_account_object, account_name_type, &custom_token_account_object::account_name> > >
      >,
      allocator<custom_token_account_object>
> token_accounts_index;

typedef multi_index_container<
      custom_token_operation_object,
      indexed_by<
            ordered_unique<tag<by_id>, member<custom_token_operation_object, custom_token_transfer_id_type, &custom_token_operation_object::id> >,
            ordered_non_unique<tag<by_account>, composite_key<custom_token_operation_object,
                  member<custom_token_operation_object, account_name_type, &custom_token_operation_object::account>,
                  member<custom_token_operation_object, uint64_t, &custom_token_operation_object::account_sequence> >,
                  composite_key_compare<std::less<account_name_type>, std::greater<uint64_t> > >,
            ordered_non_unique<tag<by_token_symbol>, composite_key<custom_token_operation_object,
                  member<custom_token_operation_object, asset_symbol_type, &custom_token_operation_object::token_symbol>,
                  member<custom_token_operation_object, uint64_t, &custom_token_operation_object::token_sequence> >,
                  composite_key_compare<std::less<asset_symbol_type>, std::greater<uint64_t> > >
      >,
      allocator<custom_token_operation_object>
> token_operation_index;

typedef multi_index_container<
      custom_token_error_object,
      indexed_by<
            ordered_unique<tag<by_id>, member<custom_token_error_object, custom_token_error_object_id_type, &custom_token_error_object::id> >,
            ordered_unique<tag<by_tx_id>, member<custom_token_error_object, transaction_id_type, &custom_token_error_object::trx_id> >,
            ordered_non_unique<tag<by_time>, member<custom_token_error_object, std::chrono::system_clock::time_point, &custom_token_error_object::time> >,
            ordered_non_unique<tag<by_token_symbol>, composite_key<custom_token_error_object,
                  member<custom_token_error_object, asset_symbol_type, &custom_token_error_object::token_symbol>,
                  member<custom_token_error_object, uint64_t, &custom_token_error_object::token_sequence> >,
                  composite_key_compare<std::less<asset_symbol_type>, std::greater<uint64_t> > >
      >,
      allocator<custom_token_error_object>
> token_error_index;

}
}
} // sophiatx::plugins::custom_tokens

FC_REFLECT(sophiatx::plugins::custom_tokens::custom_token_object,
           (id)(token_symbol)(owner_name)(total_supply)(max_supply)(burned)(paused))
FC_REFLECT(sophiatx::plugins::custom_tokens::custom_token_account_object,
           (id)(account_name)(token_symbol)(amount)(token_sequence)(account_sequence))
FC_REFLECT(sophiatx::plugins::custom_tokens::custom_token_operation_object,
           (id)(account)(token_symbol)(serialized_op)(block)(trx_id)(timestamp)(token_sequence)(account_sequence))
FC_REFLECT(sophiatx::plugins::custom_tokens::custom_token_error_object,
           (id)(trx_id)(time)(token_symbol)(error)(token_sequence))

CHAINBASE_SET_INDEX_TYPE(sophiatx::plugins::custom_tokens::custom_token_object,
                         sophiatx::plugins::custom_tokens::token_index)
CHAINBASE_SET_INDEX_TYPE(sophiatx::plugins::custom_tokens::custom_token_account_object,
                         sophiatx::plugins::custom_tokens::token_accounts_index)
CHAINBASE_SET_INDEX_TYPE(sophiatx::plugins::custom_tokens::custom_token_operation_object,
                         sophiatx::plugins::custom_tokens::token_operation_index)
CHAINBASE_SET_INDEX_TYPE(sophiatx::plugins::custom_tokens::custom_token_error_object,
                         sophiatx::plugins::custom_tokens::token_error_index)
