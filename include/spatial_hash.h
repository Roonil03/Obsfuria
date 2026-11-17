#ifndef OBSFURIA_SPATIAL_HASH_H
#define OBSFURIA_SPATIAL_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned int obsfuria_spatial_hash(int x, int y, unsigned int table_size);
int obsfuria_is_wall(int x, int y);

#ifdef __cplusplus
}
#endif

#endif
