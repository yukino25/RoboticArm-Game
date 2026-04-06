// Game/level.h
#pragma once
#include "types.h"
#include <optional>
#include <string>

// Load a level from a .level text file. Returns nullopt if file is missing or malformed.
std::optional<Level> load_level(const std::string& path);
