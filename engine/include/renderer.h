#ifndef RENDERER_H
#define RENDERER_H

// TODO: remove push constants!

// #define ENABLE_VDB_WORLD_GENERATOR
// #define ENABLE_VDB_LOD_GENERATOR
// #define ENABLE_VDB_MESH_RENDERER
#define ENABLE_DEBUG_LINE_RENDERER

#define DEBUG_LINE_VERTEX_COUNT (0xFFFFF)
#define DEBUG_LINE_INDEX_COUNT (0xFFFFF)

typedef struct time_info_t {
  float time;
  float delta_time;
} time_info_t;
typedef struct screen_info_t {
  vector2_t resolution;
} screen_info_t;
typedef struct camera_info_t {
  vector3_t position;
  float max_ray_distance;
  matrix4_t view;
  matrix4_t projection;
  matrix4_t view_projection;
  matrix4_t view_projection_inv;
  vector4_t frustum_plane[6];
} camera_info_t;

STATIC_ASSERT(ALIGNOF(time_info_t) == 4);
STATIC_ASSERT(ALIGNOF(screen_info_t) == 4);
STATIC_ASSERT(ALIGNOF(camera_info_t) == 4);

typedef struct full_screen_vertex_t {
  vector3_t position;
  int32_t reserved0;
} full_screen_vertex_t;
typedef struct debug_line_vertex_t {
  vector3_t position;
  int32_t reserved0;
  vector4_t color;
} debug_line_vertex_t;

STATIC_ASSERT(ALIGNOF(full_screen_vertex_t) == 4);
STATIC_ASSERT(ALIGNOF(debug_line_vertex_t) == 4);

typedef uint32_t full_screen_index_t;
typedef uint32_t debug_line_index_t;

typedef struct vdb_world_generator_push_constant_t {
  ivector3_t brick_position;
  int32_t brick_index;
} vdb_world_generator_push_constant_t;
typedef struct vdb_lod_generator_push_constant_t {
  ivector3_t brick_position;
  int32_t brick_index;
  int32_t brick_lod;
  int32_t reserved0;
  int32_t reserved1;
  int32_t reserved2;
} vdb_lod_generator_push_constant_t;

STATIC_ASSERT(ALIGNOF(vdb_world_generator_push_constant_t) == 4);
STATIC_ASSERT(ALIGNOF(vdb_lod_generator_push_constant_t) == 4);

typedef struct renderer_t {
  int8_t is_dirty;
  int8_t is_debug_enabled;
  int8_t rebuild_world;
  int8_t rebuild_lod;
  int32_t image_index;
  VkCommandBuffer command_buffer;
  VkSemaphore render_finished_semaphore[SWAPCHAIN_MAX_IMAGE_COUNT];
  VkSemaphore image_available_semaphore;
  VkFence frame_fence;
#ifdef ENABLE_VDB_WORLD_GENERATOR
  VkDescriptorPool vdb_world_generator_descriptor_pool;
  VkDescriptorSetLayout vdb_world_generator_descriptor_set_layout;
  VkDescriptorSet vdb_world_generator_descriptor_set;
  VkPipelineLayout vdb_world_generator_pipeline_layout;
  VkPipeline vdb_world_generator_pipeline;
#endif // ENABLE_VDB_WORLD_GENERATOR
#ifdef ENABLE_VDB_LOD_GENERATOR
  VkDescriptorPool vdb_lod_generator_descriptor_pool;
  VkDescriptorSetLayout vdb_lod_generator_descriptor_set_layout;
  VkDescriptorSet vdb_lod_generator_descriptor_set;
  VkPipelineLayout vdb_lod_generator_pipeline_layout;
  VkPipeline vdb_lod_generator_pipeline;
#endif // ENABLE_VDB_LOD_GENERATOR
#ifdef ENABLE_VDB_MESH_RENDERER
  VkDescriptorPool vdb_mesh_renderer_descriptor_pool;
  VkDescriptorSetLayout vdb_mesh_renderer_descriptor_set_layout;
  VkDescriptorSet vdb_mesh_renderer_descriptor_set;
  VkPipelineLayout vdb_mesh_renderer_pipeline_layout;
  VkPipeline vdb_mesh_renderer_pipeline;
#endif // ENABLE_VDB_MESH_RENDERER
#ifdef ENABLE_DEBUG_LINE_RENDERER
  VkDescriptorPool debug_line_descriptor_pool;
  VkDescriptorSetLayout debug_line_descriptor_set_layout;
  VkDescriptorSet debug_line_descriptor_set;
  VkPipelineLayout debug_line_pipeline_layout;
  VkPipeline debug_line_pipeline;
  buffer_t debug_line_vertex_buffer;
  buffer_t debug_line_index_buffer;
  int32_t debug_line_vertex_offset;
  int32_t debug_line_index_offset;
#endif // ENABLE_DEBUG_LINE_RENDERER
  buffer_t time_info_buffer;
  buffer_t screen_info_buffer;
  buffer_t camera_info_buffer;
  buffer_t full_screen_vertex_buffer;
  buffer_t full_screen_index_buffer;
  // TODO
  // vdb_world_generator_push_constant_t vdb_world_generator_push_constant;
  // vdb_lod_generator_push_constant_t vdb_lod_generator_push_constant;
} renderer_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern renderer_t g_renderer;

void renderer_create(void);
void renderer_draw(transform_t *transform, camera_t *camera);
void renderer_destroy(void);

#ifdef ENABLE_DEBUG_LINE_RENDERER
void renderer_draw_debug_line(vector3_t from, vector3_t to, vector4_t color);
void renderer_draw_debug_box(vector3_t position, vector3_t size, vector4_t color);
#else
#  define renderer_draw_debug_line(...)
#  define renderer_draw_debug_box(...)
#endif // ENABLE_DEBUG_LINE_RENDERER

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // RENDERER_H
