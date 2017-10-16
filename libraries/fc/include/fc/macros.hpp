#pragma once

/// Place to put some general purpose utility macros...

#define DO_PRAGMA(x) _Pragma (#x)
/// Macro to prevent unused variable warnings printed by some compilers.
#define FC_UNUSED(...) DO_PRAGMA(unused(__VA_ARGS__))