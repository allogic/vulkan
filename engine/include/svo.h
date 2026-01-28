struct node_t {
  uint32_t reserved0 : 24;
  uint32_t bit_mask : 8;

  uint32_t child_ptr : 32;

  uint32_t reserved1 : 3;
  uint32_t uv : 2;
  uint32_t normal_sign : 1;
  uint32_t normal : 2;
  uint32_t voxel_r : 5;
  uint32_t voxel_g : 6;
  uint32_t voxel_b : 5;
  uint32_t voxel_a : 8;
};
