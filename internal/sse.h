#ifndef _GRAVEYARD_INTERNAL_SSE_H_
#define _GRAVEYARD_INTERNAL_SSE_H_

#if defined(__SSE2__) ||                                                       \
    (defined(_MSC_VER) &&                                                      \
     (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 2)))
#define YOBIDUCK_HAVE_SSE2 1
#include <emmintrin.h>
#else
#define YOBIDUCK_HAVE_SSE2 0
#endif

namespace yobiduck::internal {

static constexpr bool kHaveSse2 = (YOBIDUCK_HAVE_SSE2 != 0);

} // namespace yobiduck::internal

#endif  // _GRAVEYARD_INTERNAL_SSE_H_
