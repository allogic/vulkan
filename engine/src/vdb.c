#include <pch.h>

vdb_t vdb_create(void) {
  return (vdb_t){0};
}
void vdb_destroy(vdb_t *vdb) {
}

vdb_hit_t vdb_brick_raymarch(vdb_brick_t *brick, vector3_t ray_origin, vector3_t ray_direction, float max_distance) {
  vdb_hit_t hit = {0};

  vector3_t inv_direction = vector3_inv(ray_direction);

  int8_t lod = VDB_BRICK_LOD - 1;

  float t = 0.0F;

  while (t < max_distance) {

    float cell_size = (float)(1 << lod);

    int32_t vx = (int32_t)floorf(ray_origin.x / cell_size);
    int32_t vy = (int32_t)floorf(ray_origin.y / cell_size);
    int32_t vz = (int32_t)floorf(ray_origin.z / cell_size);

    if (vx < 0 || vy < 0 || vz < 0 ||
        vx >= brick->grid_size ||
        vy >= brick->grid_size ||
        vz >= brick->grid_size) {

      break;
    }

    // int index = vx + vy * grid->size + vz * grid->size * grid->size;
    // int32_t i = vdb_brick_index(vx, vy, vz);
    int32_t v = vdb_brick_get(brick, vx, vy, vz);

    if (v == 0) {

      vector3_t next_boundary = {
        (vx + (ray_direction.x > 0.0F)) * cell_size,
        (vy + (ray_direction.y > 0.0F)) * cell_size,
        (vz + (ray_direction.z > 0.0F)) * cell_size,
      };

      float tx = (next_boundary.x - ray_origin.x) * inv_direction.x;
      float ty = (next_boundary.y - ray_origin.y) * inv_direction.y;
      float tz = (next_boundary.z - ray_origin.z) * inv_direction.z;

      float step_t = fminf(tx, fminf(ty, tz));

      t += step_t + EPSILON_4;

      ray_origin.x += ray_direction.x * (step_t + 1E-4F);
      ray_origin.y += ray_direction.y * (step_t + 1E-4F);
      ray_origin.z += ray_direction.z * (step_t + 1E-4F);

      continue;
    }

    if (lod > 0) {

      lod--;

      continue;
    }

    hit.hit = 1;
    hit.lod = lod;
    hit.position = (ivector3_t){vx, vy, vz};

    return hit;
  }

  return hit;
}
void vdb_brick_debug(vdb_brick_t *brick) {
  for (int32_t x = 0; x < brick->grid_size; x++) {
    for (int32_t y = 0; y < brick->grid_size; y++) {
      for (int32_t z = 0; z < brick->grid_size; z++) {

        vector3_t position = {(float)(brick->position.x + x), (float)(brick->position.y + y), (float)(brick->position.z + z)};
        vector3_t size = {1.0F, 1.0F, 1.0F};
        vector4_t color = {1.0F, 1.0F, 0.0F, 1.0F};

        renderer_draw_debug_box(position, size, color);
      }
    }
  }
}
