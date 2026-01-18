__forceinline float deg_to_rad(float a) {
  return a * DEG_TO_RAD;
}
__forceinline float rad_to_deg(float a) {
  return a * RAD_TO_DEG;
}
__forceinline float clamp(float a, float min, float max) {
  return fmaxf(min, fminf(max, a));
}
