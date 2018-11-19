#include <boost/test/unit_test.hpp>
#include <fc/log/logger.hpp>

#include <fc/container/flat.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/static_variant.hpp>
#include <fc/log/logger_config.hpp>

namespace fc { namespace test {

struct item;
inline bool operator == ( const item& a, const item& b );
inline bool operator < ( const item& a, const item& b );

struct item_wrapper
{
   item_wrapper() {}
   item_wrapper(item&& it) { v.reserve(1); v.insert( it ); }
   boost::container::flat_set<struct item> v;
};

inline bool operator == ( const item_wrapper& a, const item_wrapper& b )
{ return ( std::tie( a.v ) == std::tie( b.v ) ); }
inline bool operator < ( const item_wrapper& a, const item_wrapper& b )
{ return ( std::tie( a.v ) < std::tie( b.v ) ); }

struct item
{
   item(int32_t lvl = 0) : level(lvl) {}
   item(item_wrapper&& wp, int32_t lvl = 0) : level(lvl), w(wp) {}
   int32_t      level;
   item_wrapper w;
};

inline bool operator == ( const item& a, const item& b )
{ return ( std::tie( a.level, a.w ) == std::tie( b.level, b.w ) ); }
inline bool operator < ( const item& a, const item& b )
{ return ( std::tie( a.level, a.w ) < std::tie( b.level, b.w ) ); }


} } // namespace fc::test

FC_REFLECT( fc::test::item_wrapper, (v) );
FC_REFLECT( fc::test::item, (level)(w) );

BOOST_AUTO_TEST_SUITE(fc_variant_and_log)

BOOST_AUTO_TEST_CASE( types_edge_cases_test )
{
   using namespace fc::test;

   class sv : public fc::static_variant<>
   {
   public:
      BOOST_ATTRIBUTE_UNUSED typedef fc::static_variant<>::tag_type tag_type;
   };

   BOOST_TEST_MESSAGE( "========== Test empty static_variant ==========" );

   BOOST_CHECK_THROW( fc::static_variant<>(), fc::assert_exception );

   BOOST_TEST_MESSAGE( "========== Test static_variant with tag_type ==========" );

   sv::tag_type init_value = 2;
   fc::static_variant< sv::tag_type, std::string, fc::variant > variant_with_tagtype(init_value);

   BOOST_CHECK_EQUAL( variant_with_tagtype.count(), 3 );
   BOOST_CHECK_EQUAL( variant_with_tagtype.which(), 0 );

   sv::tag_type current_value = variant_with_tagtype.get<sv::tag_type>();
   BOOST_CHECK_EQUAL( current_value, init_value );
   BOOST_CHECK( variant_with_tagtype == init_value );

   for (sv::tag_type i = variant_with_tagtype.count(); i-->0;)
   {
      variant_with_tagtype.set_which(i);
      BOOST_CHECK_EQUAL(variant_with_tagtype.which(), i);
   }

   BOOST_TEST_MESSAGE( "========== Test static_variant with static_variant ==========" );

   using sv_double = fc::static_variant<double>;
   using sv_float = fc::static_variant<float>;
   fc::static_variant< sv_float, std::string, sv_double > variant;
   sv_float variant_float = 1.5f;
   variant = variant_float;
   BOOST_CHECK_EQUAL( variant.which(), 0 );
   BOOST_CHECK_EQUAL( variant.get<sv_float>().get<float>(), 1.5f );

   sv_double variant_double = 1.0;
   variant = variant_double;
   BOOST_CHECK_EQUAL( variant.which() , 2);
   BOOST_CHECK_EQUAL( variant.get<sv_double>().get<double>(), 1.0 );
}

BOOST_AUTO_TEST_SUITE_END()
