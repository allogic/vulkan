#include <pch.h>

camera_t camera_create(void) {
  return (camera_t){
    .fov = 45.0F,
    .near_z = 1.0F,
    .far_z = 1.0E+6F,
  };
}
