// Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.
#pragma once

// Because all compilers implementing GNU C extensions also has __GNUC__ defined, the only sensible way to
// figure out if we're on Clang is to exclude other GNUC-compatible compilers.
#if defined(__GNUC__)
    #if !defined(__INTEL_COMPILER)
        #if !defined(__clang__)
            #define LAVENDER_COMPILER_GCC
        #else
            #define LAVENDER_COMPILER_CLANG
        #endif
    #endif
#elif defined(_MSC_VER)
    #define LAVENDER_COMPILER_MSVC
#endif
