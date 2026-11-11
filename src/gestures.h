#pragma once

// Global event filter + gesture recognition. The filter is installed on
// the QCoreApplication instance, so it sees every delivered event (touch,
// pen/tablet, mouse, keys) before the application code does.

#include <QObject>

namespace hacks {

// Installs the global event filter. Must be called once a QCoreApplication
// exists (e.g. from the QCoreApplication::exec interposition).
void install_gestures(class QObject *app);

} // namespace hacks
