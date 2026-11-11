#pragma once

// Hack actions callable from gestures or future triggers. All actions go
// through xochitl's own Qt objects / internals resolved at runtime.

namespace hacks {

// Full-screen refresh through xochitl's own update pipeline.
void refresh_screen();

// Schedule the periodic ghost removal pass (EPFramebuffer slot).
void ghost_removal();

// Cycle EPFramebuffer::PresentationMode (Default -> Mono -> Magazine).
void cycle_presentation_mode();

// Walk the live QObject tree and dump every object's class, name and
// invokable methods to /tmp/rmhacks-objects.txt — the feature discovery
// tool for finding actions worth binding to gestures.
void dump_objects();

} // namespace hacks
