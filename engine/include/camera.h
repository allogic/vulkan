#ifndef CAMERA_H
#define CAMERA_H

typedef struct camera_t {
  float fov;
  float near_z;
  float far_z;
} camera_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

camera_t camera_create(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMERA_H
