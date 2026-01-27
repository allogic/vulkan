#ifndef MATH_H
#define MATH_H

#define PI_HALF ((float)1.57079632679489655800)
#define PI ((float)3.14159265358979323846)
#define TAU ((float)6.28318530717958623200)

#define DEG_TO_RAD ((float)0.01745329251994329547)
#define RAD_TO_DEG ((float)57.29577951308232286465)

#define EPSILON_2 ((float)1E-2)
#define EPSILON_3 ((float)1E-3)
#define EPSILON_4 ((float)1E-4)
#define EPSILON_5 ((float)1E-5)
#define EPSILON_6 ((float)1E-6)
#define EPSILON_7 ((float)1E-7)
#define EPSILON_8 ((float)1E-8)
#define EPSILON_9 ((float)1E-9)

#include <fwd.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

__forceinline float deg_to_rad(float a);
__forceinline float rad_to_deg(float a);

__forceinline float clampf(float a, float min, float max);
__forceinline int32_t clampi(int32_t a, int32_t min, int32_t max);

__forceinline float fsignf(float a);

__forceinline int32_t vec_to_index(ivector3_t position, ivector3_t size);
__forceinline ivector3_t index_to_vec(int32_t index, ivector3_t size);

#ifdef __cplusplus
}
#endif // __cplusplus

#include <math/math.inl>

#endif // MATH_H
