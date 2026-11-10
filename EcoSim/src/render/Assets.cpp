// Asset path helper implementation based on std::filesystem.
#include "render/Assets.h"

#include <filesystem>

namespace {
std::filesystem::path executableDirectory;
}  // namespace

void setExecutablePath(const std::string& argv0) {
    executableDirectory =
        std::filesystem::absolute(std::filesystem::path(argv0)).parent_path();
}

std::string assetPath(const std::string& relative) {
    return (executableDirectory / "assets" / relative).string();
}
