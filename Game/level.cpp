// Game/level.cpp
#include "level.h"
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

std::optional<Level> load_level(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return std::nullopt;

    Level level{};
    std::fill(std::begin(level.tiles), std::end(level.tiles), TileType::EMPTY);
    level.active_arm = 0;

    enum class Section { NONE, TILES, ARM, OBJECT, TARGET };
    Section section = Section::NONE;

    int tile_row = 0;
    bool has_object = false, has_target = false;
    Arm current_arm{};
    bool in_arm = false;

    auto finish_arm = [&]() {
        if (in_arm) {
            current_arm.active_segment = 0;
            level.arms.push_back(current_arm);
            current_arm = Arm{};
            in_arm = false;
        }
    };

    std::string line;
    while (std::getline(f, line)) {
        // Strip trailing \r
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Skip blank lines and top-level comments (# only before first section)
        if (line.empty()) continue;
        if (section == Section::NONE && line[0] == '#') continue;

        // Section headers
        if (line == "tiles:")  { finish_arm(); section = Section::TILES;  continue; }
        if (line == "arm:")    { finish_arm(); section = Section::ARM; in_arm = true; continue; }
        if (line == "object:") { finish_arm(); section = Section::OBJECT; continue; }
        if (line == "target:") { finish_arm(); section = Section::TARGET; continue; }

        if (section == Section::TILES) {
            if (tile_row >= (int)GAME_HEIGHT) return std::nullopt; // too many rows
            if ((int)line.size() != (int)GAME_WIDTH) return std::nullopt; // wrong width
            for (int x = 0; x < (int)GAME_WIDTH; x++) {
                if      (line[x] == '#') level.tiles[tile_row * GAME_WIDTH + x] = TileType::SOLID;
                else if (line[x] == '.') level.tiles[tile_row * GAME_WIDTH + x] = TileType::EMPTY;
                else return std::nullopt;
            }
            tile_row++;
        }
        else if (section == Section::ARM) {
            std::istringstream ss(line);
            std::string key;
            ss >> key;
            if (key == "base") {
                ss >> current_arm.base_x >> current_arm.base_y;
                if (ss.fail()) return std::nullopt;
            } else if (key == "angle") {
                ss >> current_arm.base_angle;
                if (ss.fail()) return std::nullopt;
            } else if (key == "segment") {
                std::string type_str;
                float length;
                ss >> type_str >> length;
                if (ss.fail()) return std::nullopt;
                SegmentType t;
                if      (type_str == "pivot")  t = SegmentType::PIVOT;
                else if (type_str == "extend") t = SegmentType::EXTEND;
                else if (type_str == "both")   t = SegmentType::BOTH;
                else return std::nullopt;
                // EXTEND/BOTH: start extended so j1 clears the outer sleeve.
                // max_length = 2 × sprite_width so inner/outer sprites just meet at max.
                // sprite_width per tier (sm/md/lg/xl) = 48 + tier*16.
                float start_len = (t != SegmentType::PIVOT) ? length + 24.0f : length;
                float max_len   = length; // pivots don't extend
                if (t != SegmentType::PIVOT) {
                    float sw = (length < 40.0f) ? 48.0f
                             : (length < 56.0f) ? 64.0f
                             : (length < 72.0f) ? 80.0f
                             :                    96.0f;
                    max_len = 2.0f * sw;
                }
                current_arm.segments.push_back({t, 0.0f, start_len, length, start_len, max_len});
            // TODO: add "track min max horizontal" keyword here for track-based arms
            } else {
                return std::nullopt;
            }
        }
        else if (section == Section::OBJECT) {
            std::istringstream ss(line);
            std::string key; ss >> key;
            if (key == "pos") {
                ss >> level.object.x >> level.object.y;
                if (ss.fail()) return std::nullopt;
                level.object.vx = level.object.vy = 0.0f;
                level.object.grabbed = false;
                has_object = true;
            } else return std::nullopt;
        }
        else if (section == Section::TARGET) {
            std::istringstream ss(line);
            std::string key; ss >> key;
            if (key == "pos") {
                ss >> level.target_zone.x >> level.target_zone.y
                   >> level.target_zone.w >> level.target_zone.h;
                if (ss.fail()) return std::nullopt;
                has_target = true;
            } else return std::nullopt;
        }
    }

    finish_arm();

    // Validate
    if (tile_row != (int)GAME_HEIGHT) return std::nullopt;
    if (level.arms.empty())           return std::nullopt;
    for (const auto& a : level.arms)
        if (a.segments.empty()) return std::nullopt;
    if (!has_object)                  return std::nullopt;
    if (!has_target)                  return std::nullopt;

    return level;
}
