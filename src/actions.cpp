#include "actions.h"
#include "xochitl.h"

#include <QCoreApplication>
#include <QMetaMethod>
#include <QMetaObject>
#include <QObject>
#include <QVariant>

#include <cstdio>
#include <cstdlib>

namespace hacks {

namespace {

QObject *framebuffer_object() {
  const auto &rt = xochitl::runtime();
  if (!rt.supported) {
    return nullptr;
  }
  void *fb = rt.get_instance();
  if (fb == nullptr) {
    std::fprintf(stderr, "rmhacks: getInstance returned null\n");
    return nullptr;
  }
  return reinterpret_cast<QObject *>(fb);
}

} // namespace

void refresh_screen() {
  auto *obj = framebuffer_object();
  if (obj == nullptr) {
    return;
  }
  if (const char *method = getenv("RMHACKS_REFRESH_METHOD")) {
    const bool ok =
        QMetaObject::invokeMethod(obj, method, Qt::DirectConnection);
    std::fprintf(stderr, "rmhacks: %s %s\n", method,
                 ok ? "invoked" : "FAILED (no such slot)");
    return;
  }
  const bool ok =
      QMetaObject::invokeMethod(obj, "clearGhosting", Qt::DirectConnection);
  if (ok) {
    if (xochitl::debug_enabled()) {
      std::fprintf(stderr, "rmhacks: clearGhosting invoked\n");
    }
    return;
  }

  // Fallback for experimentation: the raw internal updateRequest with
  // observed-callsite style arguments (set RMHACKS_RAW_SEND=1).
  if (xochitl::debug_enabled() || getenv("RMHACKS_RAW_SEND")) {
    const auto &rt = xochitl::runtime();
    void *fb = obj;
    alignas(void *) unsigned char region_buf[sizeof(void *)];
    rt.region_ctor(region_buf, 0, 0, 1404, 1872, 0 /* Rectangle */);
    rt.update_request(&fb, region_buf, 2 /* waveform */, 3 /* flags */, 2, "");
    if (rt.region_dtor != nullptr) {
      rt.region_dtor(region_buf);
    }
    std::fprintf(stderr, "rmhacks: raw updateRequest sent\n");
  }
}

void ghost_removal() {
  auto *obj = framebuffer_object();
  if (obj == nullptr) {
    return;
  }
  const bool ok = QMetaObject::invokeMethod(obj, "scheduleGhostRemoval",
                                            Qt::DirectConnection);
  if (xochitl::debug_enabled()) {
    std::fprintf(stderr, "rmhacks: scheduleGhostRemoval %s\n",
                 ok ? "invoked" : "FAILED");
  }
}

void cycle_presentation_mode() {
  auto *obj = framebuffer_object();
  if (obj == nullptr) {
    return;
  }
  // EPFramebuffer::PresentationMode { Default, Mono, Magazine } (moc order)
  const int current = obj->property("presentationMode").toInt();
  const int next = (current + 1) % 3;
  if (!obj->setProperty("presentationMode", next)) {
    std::fprintf(stderr,
                 "rmhacks: setting presentationMode failed "
                 "(current=%d)\n",
                 current);
    return;
  }
  std::fprintf(stderr, "rmhacks: presentationMode %d -> %d\n", current, next);
}

namespace {

void dump_object_tree(QObject *obj, int depth, std::FILE *out) {
  if (obj == nullptr || depth > 12) {
    return;
  }
  const QMetaObject *mo = obj->metaObject();
  std::fprintf(out, "%*s%s \"%s\"\n", depth * 2, "",
               mo ? mo->className() : "?", obj->objectName().toUtf8().constData());
  if (mo != nullptr) {
    for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
      const auto method = mo->method(i);
      std::fprintf(out, "%*s  %s: %s\n", depth * 2, "",
                   method.methodType() == QMetaMethod::Signal ? "signal"
                                                              : "slot",
                   method.methodSignature().constData());
    }
  }
  const auto kids = obj->children();
  for (QObject *kid : kids) {
    dump_object_tree(kid, depth + 1, out);
  }
}

} // namespace

void dump_objects() {
  QObject *app = QCoreApplication::instance();
  if (app == nullptr) {
    return;
  }
  const char *path = "/tmp/rmhacks-objects.txt";
  std::FILE *out = std::fopen(path, "w");
  if (out == nullptr) {
    std::fprintf(stderr, "rmhacks: cannot open %s\n", path);
    return;
  }
  std::fprintf(out, "# rmhacks object inventory\n");
  dump_object_tree(app, 0, out);
  std::fclose(out);
  std::fprintf(stderr, "rmhacks: object inventory written to %s\n", path);
}

} // namespace hacks
