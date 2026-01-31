/*
 * Optional Tracy profiling helpers.
 *
 * When COAL_TRACY_ENABLE is defined, this wraps Tracy's scoped zone macros.
 * Otherwise, the macros compile to no-ops.
 */

#ifndef COAL_TRACY_HH
#define COAL_TRACY_HH

#if defined(COAL_TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#define COAL_TRACY_ZONE_SCOPED ZoneScoped
#define COAL_TRACY_ZONE_SCOPED_N(name) ZoneScopedN(name)
#define COAL_TRACY_FRAME_MARK FrameMark
#else
#define COAL_TRACY_ZONE_SCOPED
#define COAL_TRACY_ZONE_SCOPED_N(name)
#define COAL_TRACY_FRAME_MARK
#endif

#endif  // COAL_TRACY_HH
