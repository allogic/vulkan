#ifndef RENDERER_H
#define RENDERER_H

typedef struct time_info_t {
  float time;
  float delta_time;
} time_info_t;
typedef struct screen_info_t {
  vector2_t resolution;
} screen_info_t;
typedef struct camera_info_t {
  vector3_t position;
  int32_t reserved;
  matrix4_t view;
  matrix4_t projection;
  matrix4_t view_projection;
  matrix4_t view_projection_inv;
} camera_info_t;

typedef struct debug_line_vertex_t {
  vector3_t position;
  vector4_t color;
} debug_line_vertex_t;

typedef uint32_t debug_line_index_t;

typedef struct renderer_t {
  int8_t is_dirty;
  int8_t is_debug_enabled;
  int32_t frames_in_flight;
  int32_t image_index;
  int32_t frame_index;
  int32_t debug_line_vertex_offset[RENDERER_MAX_FRAMES_IN_FLIGHT];
  int32_t debug_line_index_offset[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkCommandBuffer command_buffer[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkSemaphore render_finished_semaphore[RENDERER_MAX_IMAGE_COUNT];
  VkSemaphore image_available_semaphore[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkFence frame_fence[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkDescriptorPool debug_line_descriptor_pool;
  VkDescriptorSetLayout debug_line_descriptor_set_layout;
  VkDescriptorSet *debug_line_descriptor_set;
  VkPipelineLayout debug_line_pipeline_layout;
  VkPipeline debug_line_pipeline;
  VkBuffer time_info_buffer[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkBuffer screen_info_buffer[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkBuffer camera_info_buffer[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkBuffer debug_line_vertex_buffer[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkBuffer debug_line_index_buffer[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkDeviceMemory time_info_buffer_device_memory[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkDeviceMemory screen_info_buffer_device_memory[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkDeviceMemory camera_info_buffer_device_memory[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkDeviceMemory debug_line_vertex_buffer_device_memory[RENDERER_MAX_FRAMES_IN_FLIGHT];
  VkDeviceMemory debug_line_index_buffer_device_memory[RENDERER_MAX_FRAMES_IN_FLIGHT];
  debug_line_vertex_t *debug_line_vertex[RENDERER_MAX_FRAMES_IN_FLIGHT];
  debug_line_index_t *debug_line_index[RENDERER_MAX_FRAMES_IN_FLIGHT];
  time_info_t *time_info[RENDERER_MAX_FRAMES_IN_FLIGHT];
  screen_info_t *screen_info[RENDERER_MAX_FRAMES_IN_FLIGHT];
  camera_info_t *camera_info[RENDERER_MAX_FRAMES_IN_FLIGHT];
} renderer_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern renderer_t g_renderer;

void renderer_create(int32_t frames_in_flight);
void renderer_draw(transform_t *transform, camera_t *camera);
void renderer_destroy(void);

void renderer_draw_debug_line(vector3_t from, vector3_t to, vector4_t color);
void renderer_draw_debug_box(vector3_t position, vector3_t size, vector4_t color);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // RENDERER_H
