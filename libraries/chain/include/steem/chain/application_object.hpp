#pragma once
#include <fc/fixed_string.hpp>

#include <steem/chain/steem_object_types.hpp>

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

        class application_buy_object : public object< application_buy_object_type, application_buy_object >
        {
            application_buy_object() = delete;

        public:
            template<typename Constructor, typename Allocator>
            application_buy_object( Constructor&& c, allocator< Allocator > a )
            {
                c(*this);
            };

            id_type                 id;
            account_name_type       buyer;
            string                  app_name;
            time_point_sec          created;
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


        struct by_buyer_app;
        typedef multi_index_container<
                application_buy_object,
                indexed_by<
                ordered_unique< tag< by_id >, member< application_buy_object, application_buy_id_type, &application_buy_object::id > >,
                ordered_non_unique< tag< by_name >,  member<application_buy_object, string, &application_buy_object::app_name > >,
                ordered_non_unique< tag< by_author >, member<application_buy_object, account_name_type, &application_buy_object::buyer > >,
                ordered_unique< tag< by_buyer_app >,
                   composite_key< application_buy_object,
                      member< application_buy_object, account_name_type,  &application_buy_object::buyer >,
                      member< application_buy_object, string, &application_buy_object::app_name >
                   >
                >
        >,
        allocator< application_buy_object >
        > application_buy_index;

    } }

FC_REFLECT( steem::chain::application_object, (id)(name)(author)(url)(metadata)(price_param))
CHAINBASE_SET_INDEX_TYPE( steem::chain::application_object, steem::chain::application_index )

FC_REFLECT( steem::chain::application_buy_object, (id)(buyer)(app_name)(created))
CHAINBASE_SET_INDEX_TYPE( steem::chain::application_buy_object, steem::chain::application_buy_index )
