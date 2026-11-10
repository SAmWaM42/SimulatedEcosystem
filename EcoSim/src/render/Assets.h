// Asset path helper: resolves files relative to the executable, never the
// process working directory.
#pragma once

#include <string>

// Remembers the executable path supplied by the viewer.
void setExecutablePath(const std::string& argv0);
// Returns <exeDir>/assets/<relative>. The viewer is expected to be launched by
// its actual path, not through PATH lookup.
std::string assetPath(const std::string& relative);
