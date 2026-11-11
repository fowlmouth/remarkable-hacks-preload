// rmhacks — LD_PRELOAD hacks for reMarkable 2 xochitl
//
// Bootstrap: xochitl imports QCoreApplication::exec, so interposing it
// gives a guaranteed init point after the application object exists but
// before the event loop runs. From there a global event filter is
// installed and gestures can drive xochitl-internal functions resolved
// by address (see xochitl.cpp / rmhacks.conf).
//
// Everything is best effort: on unknown firmware or when disabled the
// library does nothing and xochitl runs untouched.

#include "xochitl.h"
#include "gestures.h"

#include <QCoreApplication>
#include <dlfcn.h>
#include <cstdio>
#include <unistd.h>

namespace {

using ExecFn = int (*)(void *);

bool bootstrap_done = false;

void bootstrap() {
  if (bootstrap_done) {
    return;
  }
  bootstrap_done = true;

  const auto &rt = xochitl::runtime();
  if (!rt.is_xochitl) {
    return; // loaded into some other process; stay dormant
  }
  if (!rt.supported) {
    std::fprintf(stderr,
                 "rmhacks: unsupported xochitl %s, running unpatched\n",
                 rt.version.c_str());
    return;
  }

  QObject *app = QCoreApplication::instance();
  if (app == nullptr) {
    std::fprintf(stderr, "rmhacks: no QCoreApplication at bootstrap?\n");
    return;
  }
  hacks::install_gestures(app);
}

} // namespace

extern "C" {

// interposed: int QCoreApplication::exec()
__attribute__((visibility("default")))
int _ZN16QCoreApplication4execEv(void *self) {
  static const auto real_exec = reinterpret_cast<ExecFn>(
      dlsym(RTLD_NEXT, "_ZN16QCoreApplication4execEv"));

  bootstrap();

  if (real_exec == nullptr) {
    std::fprintf(stderr, "rmhacks: cannot resolve QCoreApplication::exec\n");
    return -1;
  }
  return real_exec(self);
}

} // extern "C"
