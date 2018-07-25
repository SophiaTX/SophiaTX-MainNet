#pragma once

namespace sophiatx { namespace protocol {

// TODO:  Rename these curves to match naming in manual.md
enum curve_id
{
   quadratic,
   quadratic_curation,
   linear,
   square_root
};

} } // sophiatx::utilities


FC_REFLECT_ENUM(
   sophiatx::protocol::curve_id,
   (quadratic)
   (quadratic_curation)
   (linear)
   (square_root)
)
