#pragma once
#include <fc/fixed_string.hpp>

//#include <steem/protocol/authority.hpp>
//#include <steem/protocol/steem_operations.hpp>

#include <steem/chain/steem_object_types.hpp>
#include <steem/chain/account_object.hpp>
//#include <steem/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace steem { namespace chain {

        class application_object : public object< application_object_type, application_object >
        {
            application_object() = delete;

        public:
            template<typename Constructor, typename Allocator>
            application_object( Constructor&& c, allocator< Allocator > a )
                    : metadata( a )
            {
                c(*this);
            };

            id_type                 id;
            string                  name;
            account_name_type       author;
            string                  url;
            shared_string           metadata;
            application_price_param price_param = none;
        };

        struct by_name;
        struct by_author;

        typedef multi_index_container<
                application_object,
                indexed_by<
                ordered_unique< tag< by_id >, member< application_object, application_id_type, &application_object::id > >,
                ordered_unique< tag< by_name >,  member<application_object, string, &application_object::name > >,
                ordered_non_unique< tag< by_author >, member<application_object, account_name_type, &application_object::author > >
        >,
        allocator< application_object >
        > application_index;


    } }

FC_REFLECT( steem::chain::application_object, (id)(name)(author)(url)(metadata)(price_param))
CHAINBASE_SET_INDEX_TYPE( steem::chain::application_object, steem::chain::application_index )
