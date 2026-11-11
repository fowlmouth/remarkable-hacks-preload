#include "gestures.h"
#include "actions.h"
#include "xochitl.h"

#include <QCoreApplication>
#include <QEvent>
#include <QEventPoint>
#include <QTouchEvent>
#include <QTabletEvent>

#include <chrono>
#include <cstdio>
#include <map>

namespace hacks {

namespace {

using clock_ = std::chrono::steady_clock;

class GestureFilter : public QObject {
public:
  explicit GestureFilter(QObject *parent) : QObject(parent) {}

  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  struct TouchTrack {
    QPointF start;
    QPointF last;
  };

  // Gesture state per receiving window. The same physical touch can be
  // delivered to several windows (observed on the home screen: two
  // windows, same point ids, different coordinate origins). Tracking
  // per window keeps each stream consistent instead of looking like a
  // 240px teleport.
  struct WindowGestures {
    std::map<int, TouchTrack> active;
    std::size_t max_active = 0;
    clock_::time_point first_touch{};
    bool moved = false;
    bool consumed = false; // long-press already acted
    qreal total_dx = 0;
    qreal total_dy = 0;
    int displacement_samples = 0;
  };

  std::map<QObject *, WindowGestures> windows;

  // when the same gesture completes in two windows at once, only act once
  clock_::time_point last_action{};

  // throttled event logging
  int logged_events = 0;

  void finish_window(QObject *watched, WindowGestures &g);
};

void GestureFilter::finish_window(QObject *watched, WindowGestures &g) {
  const auto count = g.max_active;
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          clock_::now() - g.first_touch)
          .count();
  const auto now = clock_::now();
  const bool cooled_down =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - last_action)
          .count() > 500;

  if (!g.consumed && count >= 2 && g.moved) {
    // swipe: dominant average displacement of the tracked fingers
    const qreal dx = g.total_dx / std::max(1, g.displacement_samples);
    const qreal dy = g.total_dy / std::max(1, g.displacement_samples);
    if (std::abs(dx) > 150 || std::abs(dy) > 150) {
      if (cooled_down) {
        last_action = now;
        if (std::abs(dy) >= std::abs(dx)) {
          std::fprintf(stderr, "rmhacks: %zu-finger swipe %s\n", count,
                       dy > 0 ? "down" : "up");
          if (dy > 0) {
            hacks::dump_objects();
          }
        } else {
          std::fprintf(stderr, "rmhacks: %zu-finger swipe %s\n", count,
                       dx > 0 ? "right" : "left");
        }
      }
      return;
    }
  }

  const bool is_tap = !g.consumed && !g.moved && count >= 2 && elapsed < 400;
  if (is_tap && cooled_down) {
    last_action = now;
    std::fprintf(stderr, "rmhacks: %zu-finger tap detected\n", count);
    if (count >= 3) {
      hacks::ghost_removal();
    } else {
      hacks::refresh_screen();
    }
  } else if (xochitl::debug_enabled() && count > 0) {
    std::fprintf(stderr,
                 "rmhacks: gesture window rejected in %p (%zu fingers, "
                 "%lld ms, moved=%d, tap=%d)\n",
                 static_cast<void *>(watched), count,
                 static_cast<long long>(elapsed), static_cast<int>(g.moved),
                 static_cast<int>(is_tap));
  }
}

bool GestureFilter::eventFilter(QObject *watched, QEvent *event) {
  const auto type = event->type();
  const bool debug = xochitl::debug_enabled();

  if (debug) {
    // touch/tablet events are rare: always log them in detail so input
    // differences per screen (e.g. home page vs apps) are visible
    switch (type) {
      case QEvent::TouchBegin:
      case QEvent::TouchUpdate:
      case QEvent::TouchEnd:
      case QEvent::TouchCancel: {
        const auto *te = static_cast<const QTouchEvent *>(event);
        std::fprintf(stderr, "rmhacks touch: type=%d pts=%d",
                     static_cast<int>(type),
                     static_cast<int>(te->points().size()));
        for (const auto &p : te->points()) {
          std::fprintf(stderr, " [id=%d st=%d (%.0f,%.0f)]", p.id(),
                       static_cast<int>(p.state()), p.position().x(),
                       p.position().y());
        }
        std::fprintf(stderr, "\n");
        break;
      }
      default:
        if (logged_events < 400) {
          ++logged_events;
          std::fprintf(stderr, "rmhacks evt: %d -> %s\n",
                       static_cast<int>(type),
                       watched->metaObject()
                           ? watched->metaObject()->className()
                           : "?");
        }
        break;
    }
  }

  switch (type) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
      const auto *te = static_cast<const QTouchEvent *>(event);
      const auto now = clock_::now();
      auto &g = windows[watched];

      // stale state watchdog: if fingers appear to be down for a very
      // long time, a release was lost (or a finger rests); reset so the
      // recognizer cannot die permanently
      if (!g.active.empty() &&
          std::chrono::duration_cast<std::chrono::milliseconds>(
              now - g.first_touch)
                  .count() > 3000) {
        if (xochitl::debug_enabled()) {
          std::fprintf(stderr, "rmhacks: stale touch state, resetting\n");
        }
        windows.erase(watched);
        break;
      }

      for (const auto &p : te->points()) {
        // Releases can arrive in TouchUpdate (finger lifted while others
        // remain) as well as in TouchEnd — erase them wherever they show up
        if (p.state() == QEventPoint::Released) {
          const auto it = g.active.find(p.id());
          if (it != g.active.end()) {
            g.total_dx += it->second.last.x() - it->second.start.x();
            g.total_dy += it->second.last.y() - it->second.start.y();
            ++g.displacement_samples;
            g.active.erase(it);
          }
          continue;
        }
        if (p.state() == QEventPoint::Stationary) {
          continue;
        }
        const auto it = g.active.find(p.id());
        if (it == g.active.end()) {
          if (g.active.empty()) {
            g.first_touch = now;
            g.moved = false;
          }
          g.active.emplace(p.id(),
                           TouchTrack{p.position(), p.position()});
        } else {
          it->second.last = p.position();
          const auto d = p.position() - it->second.start;
          if (d.x() * d.x() + d.y() * d.y() > 40 * 40) {
            g.moved = true;
          }
        }
      }
      g.max_active = std::max(g.max_active, g.active.size());

      // two finger long press: held still for ~700ms -> act immediately,
      // mark consumed so it cannot also become a tap on release
      if (!g.consumed && !g.moved && g.active.size() >= 2 &&
          std::chrono::duration_cast<std::chrono::milliseconds>(
              now - g.first_touch)
                  .count() > 700) {
        g.consumed = true;
        std::fprintf(stderr, "rmhacks: %zu-finger long press\n",
                     g.active.size());
        hacks::cycle_presentation_mode();
      }

      if (g.active.empty() && g.max_active > 0) {
        // all fingers are up: evaluate the gesture window
        finish_window(watched, g);
        windows.erase(watched);
      }
      break;
    }
    case QEvent::TouchCancel: {
      windows.erase(watched);
      break;
    }
    default:
      break;
  }

  // never consume; xochitl must keep working
  return QObject::eventFilter(watched, event);
}

} // namespace

void install_gestures(QObject *app) {
  static GestureFilter *filter = nullptr;
  if (filter == nullptr) {
    filter = new GestureFilter(app);
  }
  app->installEventFilter(filter);
  std::fprintf(stderr, "rmhacks: gesture filter installed\n");
}

} // namespace hacks
