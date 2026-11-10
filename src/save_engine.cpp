#include "save_engine.hpp"
#include "spatial_manager.hpp"
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <unistd.h>

namespace fs = std::filesystem;

static std::string resolveSaveDir() {
    const char* xdgData = std::getenv("XDG_DATA_HOME");
    if (xdgData && *xdgData) {
        return fs::path(xdgData) / "obsfuria";
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return fs::path(home) / ".local" / "share" / "obsfuria";
    }
    return ".";
}

std::string SaveEngine::getSavePath() {
    std::string dir = resolveSaveDir();
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec || !fs::is_directory(dir, ec) || ec) return {};
    return (fs::path(dir) / "save.bin").string();
}

std::string SaveEngine::getLegacySavePath() {
    return "./save.bin";
}

uint32_t SaveEngine::computeChecksum(const GameState& state) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&state);
    size_t sz = sizeof(GameState);
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < sz; ++i) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

bool SaveEngine::validateState(const GameState& s) {
    if (s.hp <= 0 || s.hp > 9999) return false;
    if (s.maxHp <= 0 || s.maxHp > 9999) return false;
    if (s.hp > s.maxHp) return false;
    if (s.atk < 0 || s.atk > 9999) return false;
    if (s.def < 0 || s.def > 9999) return false;
    if (s.chanceToRun < 0 || s.chanceToRun > 100) return false;
    if (s.x < 0 || s.x >= 20 || s.y < 0 || s.y >= 20) return false;
    if (s.difficulty != Difficulty::EASY && s.difficulty != Difficulty::HARD) return false;
    return true;
}

bool SaveEngine::loadLegacy(const std::string& path, GameState& state) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    
    std::vector<uint8_t> buffer(sizeof(GameState));
    in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    if (in.gcount() != static_cast<std::streamsize>(sizeof(GameState))) return false;
    
    SpatialManager::applyCrypto(buffer, 0xAA);
    std::memcpy(&state, buffer.data(), sizeof(GameState));

    // Legacy saves did not reliably persist difficulty. Migration has always
    // normalized them to Easy, so do that before validating the new enum field.
    state.difficulty = Difficulty::EASY;
    if (!validateState(state)) return false;
    return true;
}

bool SaveEngine::saveState(const GameState& state) {
    if (!validateState(state)) return false;

    std::string path = getSavePath();
    if (path.empty()) return false;
    std::string tmpPath = path + ".tmp";
    
    SaveFile file;
    file.magic = SaveFile::MAGIC;
    file.version = SaveFile::VERSION;
    file.state = state;
    file.checksum = computeChecksum(state);
    
    std::vector<uint8_t> buffer(sizeof(SaveFile));
    std::memcpy(buffer.data(), &file, sizeof(SaveFile));
    
    SpatialManager::applyCrypto(buffer, 0xAA);
    
    FILE* fp = std::fopen(tmpPath.c_str(), "wb");
    if (!fp) return false;
    bool writeOk = std::fwrite(buffer.data(), 1, buffer.size(), fp) == buffer.size();
    if (std::fflush(fp) != 0) writeOk = false;
    int fd = fileno(fp);
    if (fd < 0 || fsync(fd) != 0) writeOk = false;
    if (std::fclose(fp) != 0) writeOk = false;
    if (!writeOk) {
        std::remove(tmpPath.c_str());
        return false;
    }
    
    if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {
        std::remove(tmpPath.c_str());
        return false;
    }
    return true;
}

bool SaveEngine::loadState(GameState& state) {
    std::string path = getSavePath();
    if (path.empty()) return false;
    
    if (!fs::exists(path)) {
        std::string legacy = getLegacySavePath();
        if (fs::exists(legacy)) {
            GameState legacyState;
            if (loadLegacy(legacy, legacyState)) {
                if (saveState(legacyState)) {
                    state = legacyState;
                    std::remove(legacy.c_str());
                    return true;
                }
                return false;
            }
        }
        return false;
    }
    
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    
    std::vector<uint8_t> buffer(sizeof(SaveFile));
    in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    if (in.gcount() != static_cast<std::streamsize>(sizeof(SaveFile))) return false;
    
    SpatialManager::applyCrypto(buffer, 0xAA);
    
    SaveFile file;
    std::memcpy(&file, buffer.data(), sizeof(SaveFile));
    
    if (file.magic != SaveFile::MAGIC) return false;
    if (file.version != SaveFile::VERSION) return false;
    if (file.checksum != computeChecksum(file.state)) return false;
    if (!validateState(file.state)) return false;
    
    state = file.state;
    return true;
}

bool SaveEngine::hasValidSave() {
    std::string path = getSavePath();
    if (path.empty()) return false;
    if (!fs::exists(path)) {
        GameState legacyState;
        return loadLegacy(getLegacySavePath(), legacyState);
    }
    
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    
    std::vector<uint8_t> buffer(sizeof(SaveFile));
    in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    if (in.gcount() != static_cast<std::streamsize>(sizeof(SaveFile))) return false;
    
    SpatialManager::applyCrypto(buffer, 0xAA);
    
    SaveFile file;
    std::memcpy(&file, buffer.data(), sizeof(SaveFile));
    
    return file.magic == SaveFile::MAGIC 
        && file.version == SaveFile::VERSION 
        && file.checksum == computeChecksum(file.state)
        && validateState(file.state);
}

void SaveEngine::deleteSave() {
    const std::string path = getSavePath();
    if (!path.empty()) std::remove(path.c_str());
    std::remove(getLegacySavePath().c_str());
}
