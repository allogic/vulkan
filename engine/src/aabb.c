#include <pch.h>

int8_t ray_aabb_intersect(vector3_t ray_origin, vector3_t ray_direction, vector3_t box_min, vector3_t box_max, float *t_enter, float *t_exit) {
  float t_min = -FLT_MAX;
  float t_max = FLT_MAX;

  if (fabsf(ray_direction.x) < EPSILON_6) {

    if (ray_origin.x < box_min.x || ray_origin.x > box_max.x) {
      return 0;
    }

  } else {

    float ray_direction_inv = 1.0f / ray_direction.x;
    float t1 = (box_min.x - ray_origin.x) * ray_direction_inv;
    float t2 = (box_max.x - ray_origin.x) * ray_direction_inv;

    if (t1 > t2) {
      float tmp = t1;
      t1 = t2;
      t2 = tmp;
    }

    if (t1 > t_min) {
      t_min = t1;
    }

    if (t2 < t_max) {
      t_max = t2;
    }

    if (t_min > t_max) {
      return 0;
    }
  }

  if (fabsf(ray_direction.y) < EPSILON_6) {

    if (ray_origin.y < box_min.y || ray_origin.y > box_max.y) {
      return 0;
    }

  } else {

    float ray_direction_inv = 1.0f / ray_direction.y;
    float t1 = (box_min.y - ray_origin.y) * ray_direction_inv;
    float t2 = (box_max.y - ray_origin.y) * ray_direction_inv;

    if (t1 > t2) {
      float tmp = t1;
      t1 = t2;
      t2 = tmp;
    }

    if (t1 > t_min) {
      t_min = t1;
    }

    if (t2 < t_max) {
      t_max = t2;
    }

    if (t_min > t_max) {
      return 0;
    }
  }

  if (fabsf(ray_direction.z) < EPSILON_6) {

    if (ray_origin.z < box_min.z || ray_origin.z > box_max.z) {
      return 0;
    }

  } else {

    float ray_direction_inv = 1.0f / ray_direction.z;
    float t1 = (box_min.z - ray_origin.z) * ray_direction_inv;
    float t2 = (box_max.z - ray_origin.z) * ray_direction_inv;

    if (t1 > t2) {
      float tmp = t1;
      t1 = t2;
      t2 = tmp;
    }

    if (t1 > t_min) {
      t_min = t1;
    }

    if (t2 < t_max) {
      t_max = t2;
    }

    if (t_min > t_max) {
      return 0;
    }
  }

  *t_enter = t_min;
  *t_exit = t_max;

  return 1;
}
