#pragma once
#include <fc/fixed_string.hpp>

#include <sophiatx/chain/sophiatx_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace sophiatx { namespace chain {

        class application_object : public object< application_object_type, application_object >
        {
            application_object() = delete;

        public:
            template<typename Constructor, typename Allocator>
            application_object( Constructor&& c, allocator< Allocator > a )
                    :  url(a), metadata( a )
            {
                c(*this);
            };

            id_type                 id;
            application_name_type                  name;
            account_name_type       author;
            shared_string                  url;
            shared_string           metadata;
            application_price_param price_param = none;
        };

        class application_buying_object : public object< application_buying_object_type, application_buying_object >
        {
            application_buying_object() = delete;

        public:
            template<typename Constructor, typename Allocator>
            application_buying_object( Constructor&& c, allocator< Allocator > a )
            {
                c(*this);
            };

            id_type                 id;
            account_name_type       buyer;
            application_id_type     app_id;
            time_point_sec          created;
        };

        struct by_name;
        struct by_author;

        typedef multi_index_container<
                application_object,
                indexed_by<
                ordered_unique< tag< by_id >, member< application_object, application_id_type, &application_object::id > >,
                ordered_unique< tag< by_name >,  member<application_object, application_name_type, &application_object::name > >,
                ordered_non_unique< tag< by_author >, member<application_object, account_name_type, &application_object::author > >
        >,
        allocator< application_object >
        > application_index;


        struct by_buyer_app;
        struct by_app_id;

        typedef multi_index_container<
                application_buying_object,
                indexed_by<
                ordered_unique< tag< by_id >, member< application_buying_object, application_buying_id_type, &application_buying_object::id > >,
                ordered_non_unique< tag< by_app_id >,  member<application_buying_object, application_id_type, &application_buying_object::app_id > >,
                ordered_non_unique< tag< by_author >, member<application_buying_object, account_name_type, &application_buying_object::buyer > >,
                ordered_unique< tag< by_buyer_app >,
                   composite_key< application_buying_object,
                      member< application_buying_object, account_name_type,  &application_buying_object::buyer >,
                      member< application_buying_object, application_id_type, &application_buying_object::app_id >
                   >
                >
        >,
        allocator< application_buying_object >
        > application_buying_index;

    } }

FC_REFLECT( sophiatx::chain::application_object, (id)(name)(author)(url)(metadata)(price_param))
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::application_object, sophiatx::chain::application_index )

FC_REFLECT( sophiatx::chain::application_buying_object, (id)(buyer)(app_id)(created))
CHAINBASE_SET_INDEX_TYPE( sophiatx::chain::application_buying_object, sophiatx::chain::application_buying_index )
