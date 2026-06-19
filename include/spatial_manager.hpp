#ifndef SPATIAL_MANAGER_HPP
#define SPATIAL_MANAGER_HPP

#include <vector>
#include <cstdint>

/*
 * C++ interface wrapper for NASM spatial and cryptographic routines.
 */
class SpatialManager {
public:
    /*
     * Calculates the spatial hash index for a given coordinate.
     */
    static unsigned int calculateHash(int x, int y, unsigned int tableSize);

    /*
     * Applies the rolling XOR block encryption/decryption in-place on the buffer.
     */
    static void applyCrypto(std::vector<uint8_t>& buffer, uint8_t key);
    
    /*
     * Determines if a wall exists at a given coordinate using the hash.
     */
    static bool isWall(int x, int y);
};

#endif // SPATIAL_MANAGER_HPP
