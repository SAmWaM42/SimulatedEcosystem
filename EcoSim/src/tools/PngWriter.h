// Minimal dependency-free 8-bit RGB PNG writer used for screenshot output.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Writes top-row-first RGB bytes as a valid PNG using stored deflate blocks.
// Returns false for invalid dimensions, too-short pixel data, or file errors.
bool writePng(const std::string& path, int width, int height,
              const std::vector<uint8_t>& rgb);
