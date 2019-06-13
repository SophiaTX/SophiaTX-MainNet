#ifndef SOPHIATX_PROTOCOL_CONFIG_HPP
#define SOPHIATX_PROTOCOL_CONFIG_HPP

#include <sophiatx/protocol/asset_symbol.hpp>
#include <sophiatx/protocol/asset.hpp>
#include <sophiatx/protocol/config.hpp>


#include <fc/variant_object.hpp>
#include <fc/exception/exception.hpp>


namespace sophiatx { namespace protocol {

class protocol_config {
public:
    inline static void init(const fc::mutable_variant_object& conf)
    {
        instance().config_loaded_ = true;
        try {
            instance().config_["SOPHIATX_MIN_BLOCK_SIZE_LIMIT"] = conf["SOPHIATX_MAX_TRANSACTION_SIZE"] * 16;
        } catch (...) {
            instance().config_["SOPHIATX_MIN_BLOCK_SIZE_LIMIT"] = SOPHIATX_MAX_TRANSACTION_SIZE * 16;
        }

        try {
            instance().config_["SOPHIATX_BLOCK_INTERVAL"] = conf["SOPHIATX_BLOCK_INTERVAL"];
        } catch (...) {
            instance().config_["SOPHIATX_BLOCK_INTERVAL"] = SOPHIATX_BLOCK_INTERVAL;
        }

        try {
            instance().config_["SOPHIATX_MAX_BLOCK_SIZE"] = conf["SOPHIATX_MAX_BLOCK_SIZE"];
        } catch (...) {
            instance().config_["SOPHIATX_MAX_BLOCK_SIZE"] = SOPHIATX_MAX_TRANSACTION_SIZE * SOPHIATX_BLOCK_INTERVAL * 2048;
        }

        try {
            instance().config_["SOPHIATX_MIN_ACCOUNT_CREATION_FEE"] = conf["SOPHIATX_MIN_ACCOUNT_CREATION_FEE"];
        } catch (...) {
            instance().config_["SOPHIATX_MIN_ACCOUNT_CREATION_FEE"] = SOPHIATX_MIN_ACCOUNT_CREATION_FEE;
        }

        try {
            instance().config_["SOPHIATX_SYMBOL"] = conf["SOPHIATX_SYMBOL"];
        } catch (...) {
            instance().config_["SOPHIATX_SYMBOL"] = SOPHIATX_SYMBOL;
        }

        try {
            instance().config_["BASE_FEE"] = conf["BASE_FEE"];
        } catch (...) {
            instance().config_["BASE_FEE"] = BASE_FEE;
        }

        try {
            instance().config_["BASE_FEE_SBD1"] = conf["BASE_FEE_SBD1"];
        } catch (...) {
            instance().config_["BASE_FEE_SBD1"] = BASE_FEE_SBD1;
        }

        try {
            instance().config_["BASE_FEE_SBD2"] = conf["BASE_FEE_SBD2"];
        } catch (...) {
            instance().config_["BASE_FEE_SBD2"] = BASE_FEE_SBD2;
        }

        try {
            instance().config_["BASE_FEE_SBD3"] = conf["BASE_FEE_SBD3"];
        } catch (...) {
            instance().config_["BASE_FEE_SBD3"] = BASE_FEE_SBD3;
        }

        try {
            instance().config_["BASE_FEE_SBD4"] = conf["BASE_FEE_SBD4"];
        } catch (...) {
            instance().config_["BASE_FEE_SBD4"] = BASE_FEE_SBD4;
        }

        try {
            instance().config_["BASE_FEE_SBD5"] = conf["BASE_FEE_SBD5"];
        } catch (...) {
            instance().config_["BASE_FEE_SBD5"] = BASE_FEE_SBD5;
        }
    }

    template<typename T>
    inline static T get( std::string_view index ) {
        FC_ASSERT(instance().config_loaded_, "protocol_config is not initialized!");
        T type;
        fc::from_variant(instance().config_[index], type);
        return type;
    }

private:
    protocol_config() : config_loaded_(false) {}
    ~protocol_config() {}

    inline static protocol_config &instance() {
        static protocol_config instance;
        return instance;
    }

    bool config_loaded_;
    fc::mutable_variant_object config_;
};

} } // sophiatx::protocol

#endif //SOPHIATX_PROTOCOL_CONFIG_HPP
