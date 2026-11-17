#include "spatial_hash.h"

extern unsigned int safe_nasm_spatial_hash(int x, int y, unsigned int table_size);

unsigned int obsfuria_spatial_hash(int x, int y, unsigned int table_size) {
    return table_size ? safe_nasm_spatial_hash(x, y, table_size) : 0;
}

int obsfuria_is_wall(int x, int y) {
    if (x == 0 && y == 0) return 0;
    if (x < 0 || y < 0 || x >= 20 || y >= 20) return 1;
    return obsfuria_spatial_hash(x, y, 1024) % 5 == 0;
}
