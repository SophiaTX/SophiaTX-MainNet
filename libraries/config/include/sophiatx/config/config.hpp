//
// Created by Josef Sevcik on 2019-01-18.
//

#pragma once
#include <map>
#include <memory>
#include <fc/crypto/sha256.hpp>
#include <fc/static_variant.hpp>
#include <boost/program_options.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

namespace bpo = boost::program_options;

namespace sophiatx { namespace config {

namespace{
const static fc::sha256 _public_net_chain_id = fc::sha256("1a058d1a89aff240ab203abe8a429d1a1699c339032a87e70e01022842a98324");
}

class config_entry_description{
public:
   std::string name;
   std::string description;
   fc::optional<fc::variant> default_value;
   config_entry_description(){}
   config_entry_description(const std::string _name, std::string _description, fc::variant _default_value): name(_name), description(_description){
      if(!_default_value.is_null())
         default_value = _default_value;
   }
   config_entry_description(const config_entry_description& ced):name(ced.name),description(ced.description),default_value(ced.default_value){}
   config_entry_description& operator=(const config_entry_description&ced){
      name = ced.name;
      description = ced.description;
      default_value = ced.default_value;
      return *this;
   }
};

class config_entry{
public:
   fc::variant value;
};

class extended_config_entry : public config_entry_description {
public:
   fc::optional<fc::variant> value;
};

struct variables{
   std::map<fc::sha256, std::map<std::string, config_entry >> values;
};

struct descriptions{
   std::map<std::string, config_entry_description > values;
};

class configuration_manager {
public:
   static configuration_manager &get_configuration_manager() {
      static configuration_manager instance;
      return instance;
   }

   configuration_manager(configuration_manager const &) = delete;

   void operator=(configuration_manager const &) = delete;

   fc::variant get_variable(const std::string& name, const fc::sha256& chain_id = _public_net_chain_id){
      try {
         return _variables.values.at(chain_id).at(name).value;
      }catch(std::out_of_range){
         return fc::variant();
      }
   };

   void set_variable(const std::string& name, const fc::variant& value, const fc::sha256& chain_id = _public_net_chain_id ){
      _variables.values[chain_id][name].value = value;
   };

   void set_description(const std::string& name, const fc::variant& default_value, const std::string description ){
      FC_ASSERT( _descriptions.values.find(name) == _descriptions.values.end(), "description already exists" );
      _descriptions.values[name] = config_entry_description(name, description, default_value);
   };

   std::vector<config_entry_description> get_descriptions(){
      std::vector<config_entry_description> ret;
      for(const auto& i: _descriptions.values ){
         ret.push_back(i.second);
      }
      return ret;
   }

   void load_from_file(std::string filename);
   void store_to_file(std::string filename);
   std::string print_descriptions();
   std::string print_configs();
   void merge_chain_config_with_bpo( const bpo::variables_map& bpo_map, const fc::sha256& chain_id = _public_net_chain_id );
   void merge_descriptions_with_bpo( const bpo::options_description& descs);
   void set_defaults_for_chain(const fc::sha256& chain_id);

   variables _variables;
   descriptions _descriptions;

private:
   configuration_manager() {
   };


};

}} //namespace sophiatx::config

FC_REFLECT(sophiatx::config::config_entry_description, (name)(description)(default_value))
FC_REFLECT(sophiatx::config::variables, (values))
FC_REFLECT(sophiatx::config::descriptions, (values))
FC_REFLECT(sophiatx::config::config_entry, (value))
FC_REFLECT_DERIVED(sophiatx::config::extended_config_entry, (sophiatx::config::config_entry_description), (value)  )

FC_REFLECT(sophiatx::config::configuration_manager, (_variables))

