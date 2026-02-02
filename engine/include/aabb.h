#ifndef AABB_H
#define AABB_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int8_t ray_aabb_intersect(vector3_t ray_origin, vector3_t ray_direction, vector3_t box_min, vector3_t box_max, float *t_enter, float *t_exit);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // AABB_H
