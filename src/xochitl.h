#pragma once

// Runtime view of the hosting xochitl binary: version detection, load-bias
// resolution and the function pointers used by the hacks. Everything is
// resolved lazily from /proc and the conf file; nothing is hardcoded at
// link time.

#include <cstdint>
#include <string>

namespace xochitl {

// A region must be constructed by the hosting process (Qt 6 ABI), the
// helpers below only move raw storage around.
using QRegionCtor = void (*)(void *region, int x, int y, int w, int h,
                             int type);
using QRegionDtor = void (*)(void *region);

// void updateRequest(void **fbSlot, QRegion &, int waveform, int flags,
//                    int arg4, const char *source)
using UpdateRequest = void (*)(void **, void *, int, int, int, const char *);

// void *getInstance() -> EPFramebufferSwtcon * (QObject derived)
using GetInstance = void *(*)(void);

struct Runtime {
  std::string version;
  bool is_xochitl = false;
  bool supported = false;
  std::uintptr_t load_bias = 0;

  GetInstance get_instance = nullptr;
  UpdateRequest update_request = nullptr;
  QRegionCtor region_ctor = nullptr;
  QRegionDtor region_dtor = nullptr;
};

// Populates and returns the process-wide runtime view.
const Runtime &runtime();

// Whether verbose logging is enabled (RMHACKS_DEBUG=1).
bool debug_enabled();

} // namespace xochitl
