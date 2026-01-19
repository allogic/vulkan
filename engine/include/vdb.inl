__forceinline int32_t vdb_brick_index(int32_t x, int32_t y, int32_t z) {
  return x + (y << 4) + (z << 8);
}
__forceinline int8_t vdb_brick_get(vdb_brick_t *brick, int32_t x, int32_t y, int32_t z) {
  int32_t i = vdb_brick_index(x, y, z);
  return (brick->mask[i >> 6] >> (i & 63)) & 1;
}
__forceinline void vdb_brick_set(vdb_brick_t *brick, int32_t x, int32_t y, int32_t z) {
  int32_t i = vdb_brick_index(x, y, z);
  brick->mask[i >> 6] |= (1ULL << (i & 63));
}
__forceinline void vdb_brick_clear(vdb_brick_t *brick, int32_t x, int32_t y, int32_t z) {
  int32_t i = vdb_brick_index(x, y, z);
  brick->mask[i >> 6] &= ~(1ULL << (i & 63));
}
