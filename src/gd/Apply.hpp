#pragma once
#include "../core/Settings.hpp"

namespace autodeco {

// Generate: plans from the level and adds the decoration (one editor undo step).
void runAutodecorate(Settings const& st);
// Preview: shows coloured dots where decoration would go. Adds nothing. Call again to hide.
void previewAutodecorate(Settings const& st);
// Removes the last generation if it is still the editor's newest undo step.
bool undoAutodecorate();
// Removes the last generation (if still the newest step) and generates again with st (new seed set by the caller).
void regenerateAutodecorate(Settings const& st);

}  // namespace autodeco
