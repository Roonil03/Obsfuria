#include "save_engine.hpp"
#include "spatial_manager.hpp"
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdio>

bool SaveEngine::saveState(const std::string& filename, const GameState& state) {
    std::vector<uint8_t> buffer(sizeof(GameState));
    std::memcpy(buffer.data(), &state, sizeof(GameState));
    
    SpatialManager::applyCrypto(buffer, 0xAA);
    
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return out.good();
}

bool SaveEngine::loadState(const std::string& filename, GameState& state) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) return false;
    
    std::vector<uint8_t> buffer(sizeof(GameState));
    in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    if (in.gcount() != sizeof(GameState)) return false;
    
    SpatialManager::applyCrypto(buffer, 0xAA);
    std::memcpy(&state, buffer.data(), sizeof(GameState));
    return true;
}

bool SaveEngine::hasValidSave(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in) return false;
    std::streamsize size = in.tellg();
    return size == sizeof(GameState);
}

void SaveEngine::deleteSave(const std::string& filename) {
    std::remove(filename.c_str());
}
