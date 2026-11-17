#include "spatial_manager.hpp"
#include "spatial_hash.h"
#include <cstddef>

extern "C" {
    void nasm_xor_encrypt_decrypt(uint8_t* buffer, std::size_t len, uint8_t key);
}

unsigned int SpatialManager::calculateHash(int x, int y, unsigned int tableSize) {
    return obsfuria_spatial_hash(x, y, tableSize);
}

void SpatialManager::applyCrypto(std::vector<uint8_t>& buffer, uint8_t key) {
    if (buffer.empty()) return;
    nasm_xor_encrypt_decrypt(buffer.data(), buffer.size(), key);
}

bool SpatialManager::isWall(int x, int y) {
    return obsfuria_is_wall(x, y) != 0;
}
