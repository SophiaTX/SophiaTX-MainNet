#include <boost/test/unit_test.hpp>

#include <fc/static_variant.hpp>

BOOST_AUTO_TEST_SUITE(fc_variant_and_log)

BOOST_AUTO_TEST_CASE( types_edge_cases_test )
{

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

BOOST_AUTO_TEST_CASE(mutable_variant_object_test)
{
    // no BOOST_CHECK / BOOST_REQUIRE, just see that this compiles on all supported platforms
    try {
        fc::variant v(42);
        fc::variant_object vo;
        fc::mutable_variant_object mvo;
        fc::variants vs;
        vs.push_back(fc::mutable_variant_object("level", "debug")("color", v));
        vs.push_back(fc::mutable_variant_object()("level", "debug")("color", v));
        vs.push_back(fc::mutable_variant_object("level", "debug")("color", "green"));
        vs.push_back(fc::mutable_variant_object()("level", "debug")("color", "green"));
        vs.push_back(fc::mutable_variant_object("level", "debug")(vo));
        vs.push_back(fc::mutable_variant_object()("level", "debug")(mvo));
        vs.push_back(fc::mutable_variant_object("level", "debug").set("color", v));
        vs.push_back(fc::mutable_variant_object()("level", "debug").set("color", v));
        vs.push_back(fc::mutable_variant_object("level", "debug").set("color", "green"));
        vs.push_back(fc::mutable_variant_object()("level", "debug").set("color", "green"));
    }
    FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()
