__forceinline float deg_to_rad(float a) {
  return a * DEG_TO_RAD;
}
__forceinline float rad_to_deg(float a) {
  return a * RAD_TO_DEG;
}
__forceinline float clamp(float a, float min, float max) {
  return fmaxf(min, fminf(max, a));
}
__forceinline int32_t clampi(int32_t a, int32_t min, int32_t max) {
  return (int32_t)fmaxf((float)min, fminf((float)max, (float)a));
}
