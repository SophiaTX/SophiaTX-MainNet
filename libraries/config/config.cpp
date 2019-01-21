
#include <sophiatx/config/config.hpp>
#include <boost/program_options/options_description.hpp>
#include <memory>
#include <fc/io/json.hpp>
#include <typeinfo>
#include <boost/filesystem.hpp>

using boost::any_cast;

namespace sophiatx { namespace config {

namespace{

fc::variant get_from_any(const boost::any& a)
{
   fc::variant ret;
   if( ! a.empty()) {
      //first, try int
      if( a.type() == typeid(uint16_t))
         ret = any_cast<uint16_t>(a);
      else
      if( a.type() == typeid(int16_t))
         ret = any_cast<int16_t>(a);
      else
      if( a.type() == typeid(uint32_t))
         ret = any_cast<uint32_t>(a);
      else
      if( a.type() == typeid(int32_t))
         ret = any_cast<int32_t>(a);
      else
      if( a.type() == typeid(uint64_t))
         ret = any_cast<uint64_t>(a);
      else
      if( a.type() == typeid(int64_t))
         ret = any_cast<int64_t>(a);
      else
      if( a.type() == typeid(bool))
         ret = any_cast<bool>(a);
      else //string?
      if( a.type() == typeid(std::string))
         ret = any_cast<std::string>(a);
      else
      if( a.type() == typeid(boost::filesystem::path)) {
         boost::filesystem::path tmp_ret = any_cast<boost::filesystem::path>(a);
         ret = tmp_ret.generic_string();
      }
      else //vector?
      if( a.type() == typeid(std::vector<std::string>)){
         std::vector<std::string> res;
         res = any_cast<std::vector<std::string>>(a);
         ret = res;
      }
   }
   return ret;
}


}

void configuration_manager::set_defaults_for_chain(const fc::sha256 &chain_id)
{
   for(const auto& i : _descriptions.values)
      _variables.values[chain_id][i.first].value = i.second.default_value;
}


void configuration_manager::merge_descriptions_with_bpo(const bpo::options_description &descs)
{
   for( const auto& i : descs.options() ){
      boost::any def_val;
      fc::variant vdef_val;
      if(i->semantic()->apply_default(def_val)) {
         vdef_val = get_from_any( def_val);
      }
      config_entry_description d(i->long_name(), i->description(), vdef_val );
      _descriptions.values[i->long_name()] = d;
   }
}

void configuration_manager::merge_chain_config_with_bpo(const bpo::variables_map &bpo_map,
                                                        const fc::sha256 &chain_id)
{
   for(const auto& i : bpo_map){
      const std::string& name = i.first;
      const boost::any& value = i.second.value();
      fc::variant vvalue = get_from_any(value);

      _variables.values[chain_id][name].value = vvalue;
   }
}

void configuration_manager::store_to_file(std::string filename)
{
   std::map<fc::sha256, std::map<std::string, extended_config_entry >> extended_values;
   for( const auto& chid : _variables.values ){
      for( const auto & desc : _descriptions.values ){
         extended_values[chid.first][desc.first].name = desc.second.name;
         extended_values[chid.first][desc.first].description = desc.second.description;
         extended_values[chid.first][desc.first].default_value = desc.second.default_value;
         if(chid.second.count(desc.first))
            extended_values[ chid.first ][ desc.first ].value = chid.second.at(desc.first).value;
      }

   }

   fc::variant file_config;
   fc::to_variant(extended_values, file_config);
   fc::json::save_to_file(file_config, filename, true);
}

void configuration_manager::load_from_file(std::string filename)
{
   fc::variant file_config =  fc::json::from_file(filename);
   fc::from_variant(file_config, _variables.values);
}

std::string configuration_manager::print_descriptions()
{
   fc::variant vdescriptions;
   fc::to_variant( _descriptions.values, vdescriptions);
   return fc::json::to_pretty_string(vdescriptions);
}

std::string configuration_manager::print_configs()
{
   fc::variant voptions;
   fc::to_variant( _variables.values, voptions);
   return fc::json::to_pretty_string(voptions);
}

}} //namespace sophiatx::config
