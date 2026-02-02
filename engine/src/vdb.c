#include <pch.h>

static void vdb_create_brick_data(void);
static void vdb_create_terrain_layer_buffer(void);
static void vdb_create_cluster_info_buffer(void);
static void vdb_create_occlusion_info_buffer(void);
static void vdb_create_brick_info_buffer(void);
static void vdb_create_brick_sampler(void);

static void vdb_destroy_brick_data(void);
static void vdb_destroy_buffer(void);
static void vdb_destroy_brick_sampler(void);

vdb_t g_vdb = {0};

int32_t g_vdb_axis_voxels_per_lod[VDB_LOD_COUNT_PLUS_ONE] = {
  // 256, // 256 >> 0
  // 128, // 256 >> 1
  // 64,  // 256 >> 2
  32, // 256 >> 3
  16, // 256 >> 4
  8,  // 256 >> 5
  4,  // 256 >> 6
  2,  // 256 >> 7
  1,  // 256 >> 8
};

void vdb_create(void) {
  vdb_create_brick_data();

  vdb_create_terrain_layer_buffer();
  vdb_create_cluster_info_buffer();
  vdb_create_occlusion_info_buffer();
  vdb_create_brick_info_buffer();

  vdb_create_brick_sampler();
}
void vdb_destroy(void) {
  vdb_destroy_brick_sampler();

  vdb_destroy_buffer();

  vdb_destroy_brick_data();
}

int32_t vdb_brick_position_to_index(ivector3_t brick_position) {
  return (brick_position.x) +
         (brick_position.y * VDB_CLUSTER_DIM_X) +
         (brick_position.z * VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y);
}
ivector3_t vdb_brick_index_to_position(int32_t brick_index) {
  return (ivector3_t){
    brick_index % VDB_CLUSTER_DIM_X,
    (brick_index / VDB_CLUSTER_DIM_X) % VDB_CLUSTER_DIM_Y,
    brick_index / (VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y),
  };
}

static void vdb_create_brick_data(void) {
  g_vdb.brick_image = (VkImage *)HEAP_ALLOC(sizeof(VkImage) * VDB_BRICK_COUNT, 0, 0);
  g_vdb.brick_device_memory = (VkDeviceMemory *)HEAP_ALLOC(sizeof(VkDeviceMemory) * VDB_BRICK_COUNT, 0, 0);
  g_vdb.brick_image_view = (VkImageView *)HEAP_ALLOC(sizeof(VkImageView) * VDB_BRICK_COUNT * VDB_LOD_COUNT_PLUS_ONE, 0, 0);

  VkCommandBuffer command_buffer = vkutils_begin_command_buffer();

  int32_t brick_index = 0;
  int32_t brick_count = VDB_BRICK_COUNT;

  while (brick_index < brick_count) {

    VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_3D,
      .extent = {
        .width = VDB_BRICK_SIZE,
        .height = VDB_BRICK_SIZE,
        .depth = VDB_BRICK_SIZE,
      },
      .mipLevels = VDB_LOD_COUNT_PLUS_ONE,
      .arrayLayers = 1,
      .format = VK_FORMAT_R32_UINT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateImage(g_window.device, &image_create_info, 0, &g_vdb.brick_image[brick_index]));

    VkMemoryRequirements memory_requirements = {0};

    vkGetImageMemoryRequirements(g_window.device, g_vdb.brick_image[brick_index], &memory_requirements);

    uint32_t memory_type_index = vkutils_find_memory_type_index(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &g_vdb.brick_device_memory[brick_index]));
    VK_CHECK(vkBindImageMemory(g_window.device, g_vdb.brick_image[brick_index], g_vdb.brick_device_memory[brick_index], 0));

    int32_t lod_index = 0;
    int32_t lod_count = VDB_LOD_COUNT_PLUS_ONE;

    while (lod_index < lod_count) {

      VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = g_vdb.brick_image[brick_index],
        .viewType = VK_IMAGE_VIEW_TYPE_3D,
        .format = VK_FORMAT_R32_UINT,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = lod_index,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      };

      VK_CHECK(vkCreateImageView(g_window.device, &image_view_create_info, 0, &g_vdb.brick_image_view[lod_index + (brick_index * VDB_LOD_COUNT_PLUS_ONE)]));

      lod_index++;
    }

    VkImageMemoryBarrier image_memory_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_GENERAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = g_vdb.brick_image[brick_index],
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = VDB_LOD_COUNT_PLUS_ONE,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
      .srcAccessMask = VK_ACCESS_NONE,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
    };

    vkCmdPipelineBarrier(
      command_buffer,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      0,
      0,
      0,
      0,
      0,
      1,
      &image_memory_barrier);

    brick_index++;
  }

  vkutils_end_command_buffer(command_buffer);
}

static void vdb_create_terrain_layer_buffer(void) {
  g_vdb.terrain_layer_buffer = buffer_create_uniform(0, sizeof(vdb_terrain_layer_t) * VDB_TERRAIN_LAYER_COUNT);
}
static void vdb_create_cluster_info_buffer(void) {
  vdb_cluster_info_t vdb_cluster_info = {
    .cluster_dim = {
      .x = VDB_CLUSTER_DIM_X,
      .y = VDB_CLUSTER_DIM_Y,
      .z = VDB_CLUSTER_DIM_Z,
    },
  };

  g_vdb.cluster_info_buffer = buffer_create_uniform(&vdb_cluster_info, sizeof(vdb_cluster_info_t));
}
static void vdb_create_occlusion_info_buffer(void) {
  g_vdb.occlusion_info_buffer = buffer_create_uniform_coherent(0, sizeof(vdb_occlusion_info_t)); // TODO: remove coherency..
}
static void vdb_create_brick_info_buffer(void) {
  vdb_brick_info_t *brick_info = (vdb_brick_info_t *)HEAP_ALLOC(sizeof(vdb_brick_info_t) * VDB_BRICK_COUNT, 0, 0);

  int32_t brick_index = 0;
  int32_t brick_count = VDB_BRICK_COUNT;

  while (brick_index < brick_count) {

    ivector3_t brick_position = vdb_brick_index_to_position(brick_index);

    vector3_t aabb_min = {
      (float)brick_position.x * VDB_BRICK_SIZE,
      (float)brick_position.y * VDB_BRICK_SIZE,
      (float)brick_position.z * VDB_BRICK_SIZE,
    };
    vector3_t aabb_max = {
      aabb_min.x + VDB_BRICK_SIZE,
      aabb_min.y + VDB_BRICK_SIZE,
      aabb_min.z + VDB_BRICK_SIZE,
    };

    brick_info[brick_index].aabb_min = aabb_min;
    brick_info[brick_index].aabb_max = aabb_max;
    brick_info[brick_index].lod = 0;
    brick_info[brick_index].meshlet_offset = 0;
    brick_info[brick_index].meshlet_count = 1;

    brick_index++;
  }

  g_vdb.brick_info_buffer = buffer_create_storage(brick_info, sizeof(vdb_brick_info_t));

  HEAP_FREE(brick_info);
}
static void vdb_create_brick_sampler(void) {
  VkSamplerCreateInfo sampler_create_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_NEAREST,
    .minFilter = VK_FILTER_NEAREST,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .anisotropyEnable = 0,
    .maxAnisotropy = 0.0F,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = 0,
    .compareEnable = 0,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    .mipLodBias = 0.0F,
    .minLod = 0.0F,
    .maxLod = VK_LOD_CLAMP_NONE,
  };

  VK_CHECK(vkCreateSampler(g_window.device, &sampler_create_info, 0, &g_vdb.brick_sampler));
}

static void vdb_destroy_brick_data(void) {
  int32_t brick_index = 0;
  int32_t brick_count = VDB_BRICK_COUNT;

  while (brick_index < brick_count) {

    int32_t lod_index = 0;
    int32_t lod_count = VDB_LOD_COUNT_PLUS_ONE;

    while (lod_index < lod_count) {

      vkDestroyImageView(g_window.device, g_vdb.brick_image_view[lod_index + (brick_index * VDB_LOD_COUNT_PLUS_ONE)], 0);

      lod_index++;
    }

    vkFreeMemory(g_window.device, g_vdb.brick_device_memory[brick_index], 0);
    vkDestroyImage(g_window.device, g_vdb.brick_image[brick_index], 0);

    brick_index++;
  }

  HEAP_FREE(g_vdb.brick_image);
  HEAP_FREE(g_vdb.brick_device_memory);
  HEAP_FREE(g_vdb.brick_image_view);
}

static void vdb_destroy_buffer(void) {
  buffer_destroy(&g_vdb.terrain_layer_buffer);
  buffer_destroy(&g_vdb.cluster_info_buffer);
  buffer_destroy(&g_vdb.occlusion_info_buffer);
  buffer_destroy(&g_vdb.brick_info_buffer);
}
static void vdb_destroy_brick_sampler(void) {
  vkDestroySampler(g_window.device, g_vdb.brick_sampler, 0);
}
