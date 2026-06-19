#include "../include/spatial_manager.hpp"
#include <iostream>
#include <cassert>
#include <vector>

/*
 * Validates the spatial hashing, encryption logic, and wall collision.
 */
int main() {
    std::cout << "--- STARTING PHASE 2 TEST ---" << std::endl;
    
    // Test Hash
    unsigned int hash1 = SpatialManager::calculateHash(10, 20, 1024);
    unsigned int hash2 = SpatialManager::calculateHash(10, 20, 1024);
    assert(hash1 == hash2); // Deterministic
    
    // Test Wall
    assert(SpatialManager::isWall(0, 0) == false); // Origin always safe
    assert(SpatialManager::isWall(-1, 5) == true); // Border limits
    
    // Test Crypto
    std::vector<uint8_t> data = {0x12, 0x34, 0x56, 0x78};
    std::vector<uint8_t> original = data;
    
    uint8_t key = 0xAA;
    SpatialManager::applyCrypto(data, key);
    assert(data != original);
    SpatialManager::applyCrypto(data, key);
    assert(data == original);
    
    std::cout << "Phase 2 NASM Engine Verified." << std::endl;
    return 0;
}
