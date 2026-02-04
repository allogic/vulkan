#include <pch.h>

static void vdb_create_terrain_layer_buffer(void);
static void vdb_create_cluster_info_buffer(void);
static void vdb_create_occlusion_info_buffer(void);
static void vdb_create_brick_info_buffer(void);
static void vdb_create_brick_mask_buffer(void);

vdb_t g_vdb = {0};

void vdb_create(void) {
  vdb_create_terrain_layer_buffer();
  vdb_create_cluster_info_buffer();
  vdb_create_occlusion_info_buffer();
  vdb_create_brick_info_buffer();
  vdb_create_brick_mask_buffer();
}
/*
void vdb_debug_pos_x(void) {
  const vector4_t color = {1.0f, 0.0f, 0.0f, 1.0f};

  for (int brick_i = 0; brick_i < VDB_BRICK_COUNT; brick_i++) {
    if (g_vdb.brick_mask[brick_i].any_x_faces == 0)
      continue;

    ivector3_t bp = vdb_brick_index_to_position(brick_i);

    vector3_t offset = {
      (float)bp.x * VDB_BRICK_SIZE,
      (float)bp.y * VDB_BRICK_SIZE,
      (float)bp.z * VDB_BRICK_SIZE};

    for (uint32_t slice_x = 0; slice_x < VDB_BRICK_SIZE; slice_x++) {
      if ((g_vdb.brick_mask[brick_i].any_x_faces & (1u << slice_x)) == 0)
        continue;

      uint32_t rows[VDB_BRICK_SIZE] = {0};

      for (uint32_t z = 0; z < VDB_BRICK_SIZE; z++) {
        uint32_t mask_row = 0;
        for (uint32_t y = 0; y < VDB_BRICK_SIZE; y++) {
          uint32_t idx = z * VDB_BRICK_SIZE + y + 1;
          if (g_vdb.brick_mask[brick_i].x_mask[idx] & (1u << slice_x))
            mask_row |= (1u << y);
        }
        rows[z] = mask_row;
      }

      for (uint32_t z = 0; z < VDB_BRICK_SIZE; z++) {
        uint32_t row = rows[z];
        while (row) {
          uint32_t y_start = _tzcnt_u32(row);

          uint32_t remaining = row >> y_start;
          uint32_t width = _tzcnt_u32(~remaining);

          if (y_start + width > VDB_BRICK_SIZE)
            width = VDB_BRICK_SIZE - y_start;

          uint32_t quad_mask = (width == 32) ? 0xFFFFFFFFu : (((1u << width) - 1u) << y_start);

          uint32_t height = 1;
          while (z + height < VDB_BRICK_SIZE && (rows[z + height] & quad_mask) == quad_mask)
            height++;

          float x_face = (float)slice_x + 1.0f;

          vector3_t p0 = {(float)x_face, (float)y_start, (float)z};
          vector3_t p1 = {(float)x_face, (float)(y_start + width), (float)z};
          vector3_t p2 = {(float)x_face, (float)(y_start + width), (float)(z + height)};
          vector3_t p3 = {(float)x_face, (float)y_start, (float)(z + height)};

          p0 = vector3_add(p0, offset);
          p1 = vector3_add(p1, offset);
          p2 = vector3_add(p2, offset);
          p3 = vector3_add(p3, offset);

          renderer_draw_debug_line(p0, p1, color);
          renderer_draw_debug_line(p1, p2, color);
          renderer_draw_debug_line(p2, p3, color);
          renderer_draw_debug_line(p3, p0, color);
          renderer_draw_debug_line(p0, p2, color);

          for (uint32_t hh = 0; hh < height; hh++)
            rows[z + hh] &= ~quad_mask;

          row = rows[z];
        }
      }
    }
  }
}
void vdb_debug_pos_y(void) {
  const vector4_t color = {0.0f, 1.0f, 0.0f, 1.0f};

  for (int brick_i = 0; brick_i < VDB_BRICK_COUNT; brick_i++) {
    if (g_vdb.brick_mask[brick_i].any_y_faces == 0)
      continue;

    ivector3_t bp = vdb_brick_index_to_position(brick_i);

    vector3_t offset = {
      (float)bp.x * VDB_BRICK_SIZE,
      (float)bp.y * VDB_BRICK_SIZE,
      (float)bp.z * VDB_BRICK_SIZE};

    for (uint32_t slice_y = 0; slice_y < VDB_BRICK_SIZE; slice_y++) {
      if ((g_vdb.brick_mask[brick_i].any_y_faces & (1u << slice_y)) == 0)
        continue;

      uint32_t rows[VDB_BRICK_SIZE] = {0};

      for (uint32_t z = 0; z < VDB_BRICK_SIZE; z++) {
        uint32_t mask_row = 0;
        for (uint32_t x = 0; x < VDB_BRICK_SIZE; x++) {
          uint32_t idx = z * VDB_BRICK_SIZE + x + 1;
          if (g_vdb.brick_mask[brick_i].y_mask[idx] & (1u << slice_y))
            mask_row |= (1u << x);
        }
        rows[z] = mask_row;
      }

      for (uint32_t z = 0; z < VDB_BRICK_SIZE; z++) {
        uint32_t row = rows[z];
        while (row) {
          uint32_t x_start = _tzcnt_u32(row);

          uint32_t remaining = row >> x_start;
          uint32_t width = _tzcnt_u32(~remaining);

          if (x_start + width > VDB_BRICK_SIZE)
            width = VDB_BRICK_SIZE - x_start;

          uint32_t quad_mask = (width == 32) ? 0xFFFFFFFFu : (((1u << width) - 1u) << x_start);

          uint32_t height = 1;
          while (z + height < VDB_BRICK_SIZE && (rows[z + height] & quad_mask) == quad_mask)
            height++;

          float y_face = (float)slice_y + 1.0f;

          vector3_t p0 = {(float)x_start, (float)y_face, (float)z};
          vector3_t p1 = {(float)(x_start + width), (float)y_face, (float)z};
          vector3_t p2 = {(float)(x_start + width), (float)y_face, (float)(z + height)};
          vector3_t p3 = {(float)x_start, (float)y_face, (float)(z + height)};

          p0 = vector3_add(p0, offset);
          p1 = vector3_add(p1, offset);
          p2 = vector3_add(p2, offset);
          p3 = vector3_add(p3, offset);

          renderer_draw_debug_line(p0, p1, color);
          renderer_draw_debug_line(p1, p2, color);
          renderer_draw_debug_line(p2, p3, color);
          renderer_draw_debug_line(p3, p0, color);
          renderer_draw_debug_line(p0, p2, color);

          for (uint32_t hh = 0; hh < height; hh++)
            rows[z + hh] &= ~quad_mask;

          row = rows[z];
        }
      }
    }
  }
}
void vdb_debug_pos_z(void) {
  const vector4_t color = {0.0f, 0.0f, 1.0f, 1.0f};

  for (int brick_i = 0; brick_i < VDB_BRICK_COUNT; brick_i++) {

    if (g_vdb.brick_mask[brick_i].any_z_faces == 0)
      continue;

    ivector3_t bp = vdb_brick_index_to_position(brick_i);

    vector3_t offset = {
      (float)bp.x * VDB_BRICK_SIZE,
      (float)bp.y * VDB_BRICK_SIZE,
      (float)bp.z * VDB_BRICK_SIZE};

    for (uint32_t slice_z = 0; slice_z < VDB_BRICK_SIZE; slice_z++) {
      if ((g_vdb.brick_mask[brick_i].any_z_faces & (1u << slice_z)) == 0)
        continue;

      uint32_t rows[VDB_BRICK_SIZE] = {0};

      for (uint32_t y = 0; y < VDB_BRICK_SIZE; y++) {
        uint32_t mask_row = 0;
        for (uint32_t x = 0; x < VDB_BRICK_SIZE; x++) {
          uint32_t idx = y * VDB_BRICK_SIZE + x + 1;
          if (g_vdb.brick_mask[brick_i].z_mask[idx] & (1u << slice_z))
            mask_row |= (1u << x);
        }
        rows[y] = mask_row;
      }

      for (uint32_t y = 0; y < VDB_BRICK_SIZE; y++) {
        uint32_t row = rows[y];
        while (row) {
          uint32_t x = _tzcnt_u32(row);

          uint32_t remaining = row >> x;
          uint32_t w = _tzcnt_u32(~remaining);

          if (x + w > VDB_BRICK_SIZE)
            w = VDB_BRICK_SIZE - x;

          uint32_t quad_mask = (w == 32) ? 0xFFFFFFFFu : (((1u << w) - 1u) << x);

          uint32_t h = 1;
          while (y + h < VDB_BRICK_SIZE && (rows[y + h] & quad_mask) == quad_mask)
            h++;

          float z_face = (float)slice_z + 1.0f;

          vector3_t p0 = {(float)x, (float)y, z_face};
          vector3_t p1 = {(float)(x + w), (float)y, z_face};
          vector3_t p2 = {(float)(x + w), (float)(y + h), z_face};
          vector3_t p3 = {(float)x, (float)(y + h), z_face};

          p0 = vector3_add(p0, offset);
          p1 = vector3_add(p1, offset);
          p2 = vector3_add(p2, offset);
          p3 = vector3_add(p3, offset);

          renderer_draw_debug_line(p0, p1, color);
          renderer_draw_debug_line(p1, p2, color);
          renderer_draw_debug_line(p2, p3, color);
          renderer_draw_debug_line(p3, p0, color);
          renderer_draw_debug_line(p0, p2, color);

          for (uint32_t hh = 0; hh < h; hh++)
            rows[y + hh] &= ~quad_mask;

          row = rows[y];
        }
      }
    }
  }
}
void vdb_debug_neg_x(void) {
  for (uint32_t slice_x = 0; slice_x < VDB_BRICK_SIZE; slice_x++) {
    uint32_t rows[VDB_BRICK_SIZE] = {0};

    for (uint32_t z = 0; z < VDB_BRICK_SIZE; z++)
      for (uint32_t y = 0; y < VDB_BRICK_SIZE; y++) {
        uint32_t idx = z * VDB_BRICK_SIZE + y + 1;
        if (g_vdb.brick_mask[0].x_mask[idx] & (1u << slice_x))
          rows[z] |= (1u << y);
      }

    for (uint32_t z = 0; z < VDB_BRICK_SIZE; z++) {
      while (rows[z]) {
        uint32_t y = 0;
        while (y < VDB_BRICK_SIZE && (rows[z] & (1u << y)) == 0)
          y++;
        if (y == VDB_BRICK_SIZE)
          break;

        uint32_t w = 0;
        while (y + w < VDB_BRICK_SIZE && (rows[z] & (1u << (y + w))) != 0)
          w++;

        uint32_t quad_mask = (w == 32) ? 0xFFFFFFFFu : (((1u << w) - 1u) << y);

        uint32_t h = 1;
        while (z + h < VDB_BRICK_SIZE && (rows[z + h] & quad_mask) == quad_mask)
          h++;

        float x_face = (float)slice_x;

        renderer_draw_debug_line((vector3_t){x_face, (float)y, (float)z}, (vector3_t){x_face, (float)y, (float)(z + h)}, (vector4_t){0.8f, 0.0f, 0.0f, 1});
        renderer_draw_debug_line((vector3_t){x_face, (float)y, (float)(z + h)}, (vector3_t){x_face, (float)(y + w), (float)(z + h)}, (vector4_t){0.8f, 0.0f, 0.0f, 1});
        renderer_draw_debug_line((vector3_t){x_face, (float)(y + w), (float)(z + h)}, (vector3_t){x_face, (float)(y + w), (float)z}, (vector4_t){0.8f, 0.0f, 0.0f, 1});
        renderer_draw_debug_line((vector3_t){x_face, (float)(y + w), (float)z}, (vector3_t){x_face, (float)y, (float)z}, (vector4_t){0.8f, 0.0f, 0.0f, 1});
        renderer_draw_debug_line((vector3_t){x_face, (float)y, (float)z}, (vector3_t){x_face, (float)(y + w), (float)(z + h)}, (vector4_t){0.8f, 0.0f, 0.0f, 1});

        for (uint32_t zz = 0; zz < h; zz++)
          rows[z + zz] &= ~quad_mask;
      }
    }
  }
}
void vdb_debug_neg_y(void) {
  for (uint32_t slice_y = 0; slice_y < VDB_BRICK_SIZE; slice_y++) {
    uint32_t rows[VDB_BRICK_SIZE] = {0};

    for (uint32_t z = 0; z < VDB_BRICK_SIZE; z++)
      for (uint32_t x = 0; x < VDB_BRICK_SIZE; x++) {
        uint32_t idx = z * VDB_BRICK_SIZE + x + 1;
        if (g_vdb.brick_mask[0].y_mask[idx] & (1u << slice_y))
          rows[z] |= (1u << x);
      }

    for (uint32_t z = 0; z < VDB_BRICK_SIZE; z++) {
      while (rows[z]) {
        uint32_t x = 0;
        while (x < VDB_BRICK_SIZE && (rows[z] & (1u << x)) == 0)
          x++;
        if (x == VDB_BRICK_SIZE)
          break;

        uint32_t w = 0;
        while (x + w < VDB_BRICK_SIZE && (rows[z] & (1u << (x + w))) != 0)
          w++;

        uint32_t quad_mask = (w == 32) ? 0xFFFFFFFFu : (((1u << w) - 1u) << x);

        uint32_t h = 1;
        while (z + h < VDB_BRICK_SIZE && (rows[z + h] & quad_mask) == quad_mask)
          h++;

        float y_face = (float)slice_y;

        renderer_draw_debug_line((vector3_t){(float)x, y_face, (float)z}, (vector3_t){(float)x, y_face, (float)(z + h)}, (vector4_t){0.0f, 0.7f, 0.0f, 1});
        renderer_draw_debug_line((vector3_t){(float)x, y_face, (float)(z + h)}, (vector3_t){(float)(x + w), y_face, (float)(z + h)}, (vector4_t){0.0f, 0.7f, 0.0f, 1});
        renderer_draw_debug_line((vector3_t){(float)(x + w), y_face, (float)(z + h)}, (vector3_t){(float)(x + w), y_face, (float)z}, (vector4_t){0.0f, 0.7f, 0.0f, 1});
        renderer_draw_debug_line((vector3_t){(float)(x + w), y_face, (float)z}, (vector3_t){(float)x, y_face, (float)z}, (vector4_t){0.0f, 0.7f, 0.0f, 1});
        renderer_draw_debug_line((vector3_t){(float)x, y_face, (float)z}, (vector3_t){(float)(x + w), y_face, (float)(z + h)}, (vector4_t){0.0f, 0.7f, 0.0f, 1});

        for (uint32_t zz = 0; zz < h; zz++)
          rows[z + zz] &= ~quad_mask;
      }
    }
  }
}
void vdb_debug_neg_z(void) {
  for (uint32_t slice_z = 0; slice_z < VDB_BRICK_SIZE; slice_z++) {
    uint32_t rows[VDB_BRICK_SIZE] = {0};

    for (uint32_t y = 0; y < VDB_BRICK_SIZE; y++)
      for (uint32_t x = 0; x < VDB_BRICK_SIZE; x++) {
        uint32_t idx = y * VDB_BRICK_SIZE + x + 1;
        if (g_vdb.brick_mask[0].z_mask[idx] & (1u << slice_z))
          rows[y] |= (1u << x);
      }

    for (uint32_t y = 0; y < VDB_BRICK_SIZE; y++) {
      while (rows[y]) {
        uint32_t x = 0;
        while (x < VDB_BRICK_SIZE && (rows[y] & (1u << x)) == 0)
          x++;
        if (x == VDB_BRICK_SIZE)
          break;

        uint32_t w = 0;
        while (x + w < VDB_BRICK_SIZE && (rows[y] & (1u << (x + w))) != 0)
          w++;

        uint32_t quad_mask = (w == 32) ? 0xFFFFFFFFu : (((1u << w) - 1u) << x);

        uint32_t h = 1;
        while (y + h < VDB_BRICK_SIZE && (rows[y + h] & quad_mask) == quad_mask)
          h++;

        float z_face = (float)slice_z;

        renderer_draw_debug_line((vector3_t){(float)x, (float)y, z_face}, (vector3_t){(float)x, (float)(y + h), z_face}, (vector4_t){0.0f, 0.4f, 0.8f, 1});
        renderer_draw_debug_line((vector3_t){(float)x, (float)(y + h), z_face}, (vector3_t){(float)(x + w), (float)(y + h), z_face}, (vector4_t){0.0f, 0.4f, 0.8f, 1});
        renderer_draw_debug_line((vector3_t){(float)(x + w), (float)(y + h), z_face}, (vector3_t){(float)(x + w), (float)y, z_face}, (vector4_t){0.0f, 0.4f, 0.8f, 1});
        renderer_draw_debug_line((vector3_t){(float)(x + w), (float)y, z_face}, (vector3_t){(float)x, (float)y, z_face}, (vector4_t){0.0f, 0.4f, 0.8f, 1});
        renderer_draw_debug_line((vector3_t){(float)x, (float)y, z_face}, (vector3_t){(float)(x + w), (float)(y + h), z_face}, (vector4_t){0.0f, 0.4f, 0.8f, 1});

        for (uint32_t yy = 0; yy < h; yy++)
          rows[y + yy] &= ~quad_mask;
      }
    }
  }
}
*/
void vdb_destroy(void) {
  buffer_destroy(&g_vdb.terrain_layer_buffer);
  buffer_destroy(&g_vdb.cluster_info_buffer);
  buffer_destroy(&g_vdb.occlusion_info_buffer);
  buffer_destroy(&g_vdb.brick_info_buffer);
  buffer_destroy(&g_vdb.brick_mask_buffer);
}

int32_t vdb_brick_position_to_index(ivector3_t brick_position) {
  return (brick_position.x) +
         (brick_position.y * VDB_CLUSTER_DIM_X) +
         (brick_position.z * VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y);
}
ivector3_t vdb_brick_index_to_position(int32_t brick_index) {
  return (ivector3_t){
    brick_index % VDB_CLUSTER_DIM_X,
    (brick_index / VDB_CLUSTER_DIM_X) % VDB_CLUSTER_DIM_Y,
    brick_index / (VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y),
  };
}

static void vdb_create_terrain_layer_buffer(void) {
  g_vdb.terrain_layer_buffer = buffer_create_uniform(0, sizeof(vdb_terrain_layer_t) * VDB_TERRAIN_LAYER_COUNT);
}
static void vdb_create_cluster_info_buffer(void) {
  vdb_cluster_info_t vdb_cluster_info = {
    .cluster_dim = {
      .x = VDB_CLUSTER_DIM_X,
      .y = VDB_CLUSTER_DIM_Y,
      .z = VDB_CLUSTER_DIM_Z,
    },
  };

  g_vdb.cluster_info_buffer = buffer_create_uniform(&vdb_cluster_info, sizeof(vdb_cluster_info_t));
}
static void vdb_create_occlusion_info_buffer(void) {
  g_vdb.occlusion_info_buffer = buffer_create_uniform_coherent(0, sizeof(vdb_occlusion_info_t)); // TODO: remove coherency..
}
static void vdb_create_brick_info_buffer(void) {
  vdb_brick_info_t *brick_info = (vdb_brick_info_t *)HEAP_ALLOC(sizeof(vdb_brick_info_t) * VDB_BRICK_COUNT, 1, 0);

  int32_t brick_index = 0;
  int32_t brick_count = VDB_BRICK_COUNT;

  while (brick_index < brick_count) {

    ivector3_t brick_position = vdb_brick_index_to_position(brick_index);

    vector3_t aabb_min = {
      (float)brick_position.x * VDB_BRICK_SIZE,
      (float)brick_position.y * VDB_BRICK_SIZE,
      (float)brick_position.z * VDB_BRICK_SIZE,
    };
    vector3_t aabb_max = {
      aabb_min.x + VDB_BRICK_SIZE,
      aabb_min.y + VDB_BRICK_SIZE,
      aabb_min.z + VDB_BRICK_SIZE,
    };

    brick_info[brick_index].brick_position = brick_position;
    brick_info[brick_index].lod = 0;
    brick_info[brick_index].aabb_min = aabb_min;
    brick_info[brick_index].aabb_max = aabb_max;

    brick_index++;
  }

  g_vdb.brick_info_buffer = buffer_create_storage(brick_info, sizeof(vdb_brick_info_t) * VDB_BRICK_COUNT);

  HEAP_FREE(brick_info);
}
static void vdb_create_brick_mask_buffer(void) {
  /*
  g_vdb.brick_mask = (vdb_brick_mask_t *)HEAP_ALLOC(sizeof(vdb_brick_mask_t) * VDB_BRICK_COUNT, 1, 0);

  float nx = 0.1F;
  float ny = 1.2F;
  float nz = 0.1F;
  float len_inv = 1.0F / sqrtf(nx * nx + ny * ny + nz * nz);

  nx *= len_inv;
  ny *= len_inv;
  nz *= len_inv;

  float d = -32.0F;

  int32_t brick_index = 0;
  int32_t brick_count = VDB_BRICK_COUNT;

  while (brick_index < brick_count) {

    ivector3_t brick_position = vdb_brick_index_to_position(brick_index);

    for (int lz = 0; lz < VDB_BRICK_SIZE; lz++) {
      for (int ly = 0; ly < VDB_BRICK_SIZE; ly++) {
        for (int lx = 0; lx < VDB_BRICK_SIZE; lx++) {

          float wx = brick_position.x * VDB_BRICK_SIZE + lx + 0.5f;
          float wy = brick_position.y * VDB_BRICK_SIZE + ly + 0.5f;
          float wz = brick_position.z * VDB_BRICK_SIZE + lz + 0.5f;

          float sdf = wx * nx + wy * ny + wz * nz + d;

          if (sdf < 0.0f) {
            // +Z face
            float sdf_front = wx * nx + wy * ny + (wz + 1.0f) * nz + d;
            if (sdf_front >= 0.0f) {
              uint32_t idx = ly * 32 + lx + 1;
              g_vdb.brick_mask[brick_index].z_mask[idx] |= (1u << lz);
              g_vdb.brick_mask[brick_index].any_z_faces |= (1u << lz);
            }

            // +X face
            float sdf_right = (wx + 1.0f) * nx + wy * ny + wz * nz + d;
            if (sdf_right >= 0.0f) {
              uint32_t idx = lz * 32 + ly + 1;
              g_vdb.brick_mask[brick_index].x_mask[idx] |= (1u << lx);
              g_vdb.brick_mask[brick_index].any_x_faces |= (1u << lx);
            }

            // +Y face
            float sdf_up = wx * nx + (wy + 1.0f) * ny + wz * nz + d;
            if (sdf_up >= 0.0f) {
              uint32_t idx = lz * 32 + lx + 1;
              g_vdb.brick_mask[brick_index].y_mask[idx] |= (1u << ly);
              g_vdb.brick_mask[brick_index].any_y_faces |= (1u << ly);
            }
          }
        }
      }
    }

    brick_index++;
  }
  */

  g_vdb.brick_mask_buffer = buffer_create_storage(0, sizeof(vdb_brick_mask_t) * VDB_BRICK_COUNT);
}
