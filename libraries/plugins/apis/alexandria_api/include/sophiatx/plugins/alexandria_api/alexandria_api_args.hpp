#pragma once

#include <sophiatx/plugins/alexandria_api/alexandria_api_objects.hpp>
#include <sophiatx/plugins/block_api/block_api_args.hpp>
#include <sophiatx/plugins/database_api/database_api_args.hpp>
#include <sophiatx/plugins/account_history_api/account_history_args.hpp>

#include <sophiatx/protocol/types.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

/**
 * @brief Macro defining api method _args and _return types
 */
#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;


/**
 * list_witnesses
 */
struct list_witnesses_args {
   fc::variant       start;
   uint32_t          limit;
};
struct list_witnesses_return {
   std::vector< sophiatx::protocol::account_name_type > witnesses;
};

/**
 * list_witnesses_by_vote
 */
struct list_witnesses_by_vote_args {
   std::string       name;
   uint32_t          limit;
};
struct list_witnesses_by_vote_return {
   std::vector<alexandria_api::api_witness_object> witnesses;
};

/**
 * get_witness
 */
struct get_witness_args {
   account_name_type       name;
};
struct get_witness_return {
   optional< alexandria_api::api_witness_object > witness;
};


/**
 * get_block
 */
struct get_block_return {
   optional< alexandria_api::api_signed_block > block;
};

/**
 * get_ops_in_block
 */
struct get_ops_in_block_return {
   vector< alexandria_api::api_operation_object > ops;
};

/**
 * get_feed_history
 */
struct get_feed_history_return {
   alexandria_api::api_feed_history_object history;
};

/**
 * get_applications
 */
struct get_applications_args {
   vector<std::string> names;
};
struct get_applications_return {
   vector<alexandria_api::api_application_object> applications;
};



DEFINE_API_ARGS( list_witnesses,           alexandria_api::list_witnesses_args,          alexandria_api::list_witnesses_return )
DEFINE_API_ARGS( list_witnesses_by_vote,   alexandria_api::list_witnesses_by_vote_args,  alexandria_api::list_witnesses_by_vote_return )
DEFINE_API_ARGS( git_witness,              alexandria_api::get_witness_args,             alexandria_api::get_witness_return )
DEFINE_API_ARGS( get_block,                block_api::get_block_args,                    alexandria_api::get_block_return )
DEFINE_API_ARGS( get_ops_in_block,         account_history::get_ops_in_block_args,       alexandria_api::get_ops_in_block_return)
DEFINE_API_ARGS( get_feed_history,         database_api::get_feed_history_args,          alexandria_api::get_feed_history_return)
DEFINE_API_ARGS( get_application_buyings,  database_api::get_application_buyings_args,   database_api::get_application_buyings_return)
DEFINE_API_ARGS( get_applications,         alexandria_api::get_applications_args,        alexandria_api::get_applications_return)





} } } // sophiatx::plugins::alexandria_api

FC_REFLECT( sophiatx::plugins::alexandria_api::list_witnesses_args,
            (start)(limit) )
FC_REFLECT( sophiatx::plugins::alexandria_api::list_witnesses_return,
            (witnesses) )


FC_REFLECT( sophiatx::plugins::alexandria_api::list_witnesses_by_vote_args,
            (name)(limit) )
FC_REFLECT( sophiatx::plugins::alexandria_api::list_witnesses_by_vote_return,
            (witnesses) )


FC_REFLECT( sophiatx::plugins::alexandria_api::get_witness_args,
            (name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_witness_return,
            (witness) )


FC_REFLECT( sophiatx::plugins::alexandria_api::get_block_return,
            (block) )


FC_REFLECT( sophiatx::plugins::alexandria_api::get_ops_in_block_return,
            (ops) )


FC_REFLECT( sophiatx::plugins::alexandria_api::get_feed_history_return,
            (history) )

FC_REFLECT( sophiatx::plugins::alexandria_api::get_applications_args,
            (names) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_applications_return,
            (applications) )
