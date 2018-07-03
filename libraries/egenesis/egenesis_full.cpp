//                                   _           _    __ _ _        //
//                                  | |         | |  / _(_) |       //
//    __ _  ___ _ __   ___ _ __ __ _| |_ ___  __| | | |_ _| | ___   //
//   / _` |/ _ \ '_ \ / _ \ '__/ _` | __/ _ \/ _` | |  _| | |/ _ \  //
//  | (_| |  __/ | | |  __/ | | (_| | ||  __/ (_| | | | | | |  __/  //
//   \__, |\___|_| |_|\___|_|  \__,_|\__\___|\__,_| |_| |_|_|\___|  //
//    __/ |                                                         //
//   |___/                                                          //
//                                                                  //
// Generated by:  libraries/chain_id/identify_chain.cpp             //
//                                                                  //
// Warning: This is a generated file, any changes made here will be //
// overwritten by the build process.  If you need to change what    //
// is generated here, you should use the CMake variable             //
// SOPHIATX_EGENESIS_JSON to specify an embedded genesis state.     //
//                                                                  //

/*
 * Copyright (c) 2015, Cryptonomex, Inc.
 * All rights reserved.
 *
 * This source code is provided for evaluation in private test networks only, until September 8, 2015. After this date, this license expires and
 * the code may not be used, modified or distributed for any purpose. Redistribution and use in source and binary forms, with or without modification,
 * are permitted until September 8, 2015, provided that the following conditions are met:
 *
 * 1. The code and/or derivative works are used only for private test networks consisting of no more than 10 P2P nodes.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sophiatx/protocol/types.hpp>
#include <sophiatx/egenesis/egenesis.hpp>

namespace sophiatx { namespace egenesis {

using namespace sophiatx::chain;

static const char genesis_json_array[6][40+1] =
{
"{\n   \"initial_public_key\" : \"SPH8Xg6cEbq",
"PCY8jrWFccgbCq5Fjw1okivwwmLDDgqQCQeAk7je",
"du\",\n   \"initial_balace\" : 3500000000000",
"00,\n   \"initial_accounts\" : [],\n   \"gene",
"sis_time\" : \"2018-07-03T14:00:00\",\n   \"i",
"nitial_chain_id\" : \"\"\n}\n"
};

chain_id_type get_egenesis_chain_id()
{
   return chain_id_type( "7ec0065f701f7cc8257296d5408105d49333d5ea03cf167dda73bbd64b83146c" );
}

void compute_egenesis_json( std::string& result )
{
   result.reserve( 224 );
   result.resize(0);
   for( size_t i=0; i<6-1; i++ )
   {
      result.append( genesis_json_array[i], 40 );
   }
   result.append( std::string( genesis_json_array[ 6-1 ] ) );
   return;
}

fc::sha256 get_egenesis_json_hash()
{
   return fc::sha256( "7ec0065f701f7cc8257296d5408105d49333d5ea03cf167dda73bbd64b83146c" );
}

} }