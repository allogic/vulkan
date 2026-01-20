#ifndef MUTILS_H
#define MUTILS_H

#define PI_HALF ((float)1.57079632679489655800)
#define PI ((float)3.14159265358979323846)
#define TAU ((float)6.28318530717958623200)

#define DEG_TO_RAD ((float)0.01745329251994329547)
#define RAD_TO_DEG ((float)57.29577951308232286465)

#define EPSILON_3 ((float)1E-3)
#define EPSILON_4 ((float)1E-4)
#define EPSILON_5 ((float)1E-5)
#define EPSILON_6 ((float)1E-6)

#include <fwd.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

__forceinline float deg_to_rad(float a);
__forceinline float rad_to_deg(float a);

__forceinline float clamp(float a, float min, float max);
__forceinline int32_t clampi(int32_t a, int32_t min, int32_t max);

#ifdef __cplusplus
}
#endif // __cplusplus

#include <mutils.inl>

#endif // MUTILS_H
