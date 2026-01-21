__forceinline int32_t vdb_brick_cell_count(int8_t lod) {
  switch (lod) {
    case 0: {
      return VDB_BRICK_CELL_COUNT_LOD0;
    }
    case 1: {
      return VDB_BRICK_CELL_COUNT_LOD1;
    }
    case 2: {
      return VDB_BRICK_CELL_COUNT_LOD2;
    }
    case 3: {
      return VDB_BRICK_CELL_COUNT_LOD3;
    }
    case 4: {
      return VDB_BRICK_CELL_COUNT_LOD4;
    }
    case 5: {
      return VDB_BRICK_CELL_COUNT_LOD5;
    }
    case 6: {
      return VDB_BRICK_CELL_COUNT_LOD6;
    }
  }

  return -1;
}
__forceinline int32_t vdb_brick_cell_size(int8_t lod) {
  return 1 << lod;
}
__forceinline int32_t vdb_brick_index(int8_t lod, int32_t x, int32_t y, int32_t z) {
  switch (lod) {
    case 0: {
      return x + (y << 6) + (z << 12);
    }
    case 1: {
      return x + (y << 5) + (z << 10);
    }
    case 2: {
      return x + (y << 4) + (z << 8);
    }
    case 3: {
      return x + (y << 3) + (z << 6);
    }
    case 4: {
      return x + (y << 2) + (z << 4);
    }
    case 5: {
      return x + (y << 1) + (z << 2);
    }
    case 6: {
      return 0;
    }
  }

  return -1;
}
__forceinline int8_t vdb_brick_get(vdb_brick_t *brick, int8_t lod, int32_t x, int32_t y, int32_t z) {
  int32_t i = vdb_brick_index(lod, x, y, z);

  switch (lod) {
    case 0: {
      return (brick->mask_lod0[i >> 6] >> (i & 63)) & 1;
    }
    case 1: {
      return (brick->mask_lod1[i >> 6] >> (i & 63)) & 1;
    }
    case 2: {
      return (brick->mask_lod2[i >> 6] >> (i & 63)) & 1;
    }
    case 3: {
      return (brick->mask_lod3[i >> 6] >> (i & 63)) & 1;
    }
    case 4: {
      return (brick->mask_lod4[i >> 6] >> (i & 63)) & 1;
    }
    case 5: {
      return (brick->mask_lod5[0] >> i) & 1;
    }
    case 6: {
      return (brick->mask_lod6[0]) & 1;
    }
  }

  return 0;
}
__forceinline void vdb_brick_set(vdb_brick_t *brick, int8_t lod, int32_t x, int32_t y, int32_t z) {
  int32_t i = vdb_brick_index(lod, x, y, z);

  switch (lod) {
    case 0: {
      brick->mask_lod0[i >> 6] |= (1ULL << (i & 63));

      break;
    }
    case 1: {
      brick->mask_lod1[i >> 6] |= (1ULL << (i & 63));

      break;
    }
    case 2: {
      brick->mask_lod2[i >> 6] |= (1ULL << (i & 63));

      break;
    }
    case 3: {
      brick->mask_lod3[i >> 6] |= (1ULL << (i & 63));

      break;
    }
    case 4: {
      brick->mask_lod4[i >> 6] |= (1ULL << (i & 63));

      break;
    }
    case 5: {
      brick->mask_lod5[0] |= (1ULL << (i & 63));

      break;
    }
    case 6: {
      brick->mask_lod6[0] |= 1;

      break;
    }
  }
}
__forceinline void vdb_brick_clear(vdb_brick_t *brick, int8_t lod, int32_t x, int32_t y, int32_t z) {
  int32_t i = vdb_brick_index(lod, x, y, z);

  switch (lod) {
    case 0: {
      brick->mask_lod0[i >> 6] &= ~(1ULL << (i & 63));

      break;
    }
    case 1: {
      brick->mask_lod1[i >> 6] &= ~(1ULL << (i & 63));

      break;
    }
    case 2: {
      brick->mask_lod2[i >> 6] &= ~(1ULL << (i & 63));

      break;
    }
    case 3: {
      brick->mask_lod3[i >> 6] &= ~(1ULL << (i & 63));

      break;
    }
    case 4: {
      brick->mask_lod4[i >> 6] &= ~(1ULL << (i & 63));

      break;
    }
    case 5: {
      brick->mask_lod5[0] &= ~(1ULL << (i & 63));

      break;
    }
    case 6: {
      brick->mask_lod6[0] &= ~(1);

      break;
    }
  }
}
