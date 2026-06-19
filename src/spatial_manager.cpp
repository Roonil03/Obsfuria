#include "spatial_manager.hpp"

extern "C" {
    unsigned int safe_nasm_spatial_hash(int x, int y, unsigned int tableSize);
    void nasm_xor_encrypt_decrypt(uint8_t* buffer, unsigned int len, uint8_t key);
}

unsigned int SpatialManager::calculateHash(int x, int y, unsigned int tableSize) {
    if (tableSize == 0) return 0;
    return safe_nasm_spatial_hash(x, y, tableSize);
}

void SpatialManager::applyCrypto(std::vector<uint8_t>& buffer, uint8_t key) {
    if (buffer.empty()) return;
    nasm_xor_encrypt_decrypt(buffer.data(), buffer.size(), key);
}

/*
 * A wall exists if the hash mod 5 is 0. But we always leave 0,0 clear.
 */
bool SpatialManager::isWall(int x, int y) {
    if (x == 0 && y == 0) return false;
    // Bound the maze size conceptually
    if (x < 0 || y < 0 || x >= 20 || y >= 20) return true; // borders
    return (calculateHash(x, y, 1024) % 5) == 0;
}
