#include <pch.h>

static vdb_brick_t vdb_create_brick(void);
static void vdb_destroy_brick(vdb_brick_t *brick);

vdb_t g_vdb = {0};

void vdb_create(void) {
  vdb_info_t vdb_info = {
    .cluster_dim = {
      .x = VDB_CLUSTER_DIM_X,
      .y = VDB_CLUSTER_DIM_Y,
      .z = VDB_CLUSTER_DIM_Z,
    },
  };

  g_vdb.brick = (vdb_brick_t *)HEAP_ALLOC(sizeof(vdb_brick_t), 0, 0);

  g_vdb.info_buffer = buffer_create_uniform(&vdb_info, sizeof(vdb_info_t));
  g_vdb.layer_buffer = buffer_create_uniform_coherent(0, sizeof(vdb_layer_t) * VDB_MAX_LAYER);

  int32_t brick_index = 0;
  int32_t brick_count = VDB_BRICK_COUNT;

  while (brick_index < brick_count) {

    g_vdb.brick[brick_index] = vdb_create_brick();

    brick_index++;
  }
}
void vdb_destroy(void) {
  int32_t brick_index = 0;
  int32_t brick_count = VDB_BRICK_COUNT;

  while (brick_index < brick_count) {

    vdb_destroy_brick(&g_vdb.brick[brick_index]);

    brick_index++;
  }

  buffer_destroy(&g_vdb.info_buffer);
  buffer_destroy(&g_vdb.layer_buffer);

  HEAP_FREE(g_vdb.brick);
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

static vdb_brick_t vdb_create_brick(void) {
  vdb_brick_t brick = {0};

  VkImageCreateInfo image_create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_3D,
    .extent = {
      .width = VDB_BASE_RES,
      .height = VDB_BASE_RES,
      .depth = VDB_BASE_RES,
    },
    .mipLevels = VDB_MAX_LOD,
    .arrayLayers = 1,
    .format = VK_FORMAT_R32_UINT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  VK_CHECK(vkCreateImage(g_window.device, &image_create_info, 0, &brick.image));

  VkMemoryRequirements memory_requirements = {0};

  vkGetImageMemoryRequirements(g_window.device, brick.image, &memory_requirements);

  VkMemoryAllocateInfo memory_allocate_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size,
    .memoryTypeIndex = vkutils_find_memory_type_index(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
  };

  VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &brick.device_memory));
  VK_CHECK(vkBindImageMemory(g_window.device, brick.image, brick.device_memory, 0));

  int32_t lod_index = 0;
  int32_t lod_count = VDB_MAX_LOD;

  while (lod_index < lod_count) {

    VkImageViewCreateInfo image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = brick.image,
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

    VK_CHECK(vkCreateImageView(g_window.device, &image_view_create_info, 0, &brick.image_view[lod_index]));

    VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .anisotropyEnable = 1,
      .maxAnisotropy = g_window.max_sampler_anisotropy,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = 0,
      .compareEnable = 0,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .mipLodBias = 0.0F,
      .minLod = 0.0F,
      .maxLod = 0.0F,
    };

    VK_CHECK(vkCreateSampler(g_window.device, &sampler_create_info, 0, &brick.sampler[lod_index]));

    lod_index++;
  }

  VkImageMemoryBarrier image_memory_barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_GENERAL,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = brick.image,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = VDB_MAX_LOD,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    .srcAccessMask = VK_ACCESS_NONE,
    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
  };

  VkCommandBuffer command_buffer = vkutils_begin_command_buffer();

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

  vkutils_end_command_buffer(command_buffer);

  return brick;
}
static void vdb_destroy_brick(vdb_brick_t *brick) {
  int32_t lod_index = 0;
  int32_t lod_count = VDB_MAX_LOD;

  while (lod_index < lod_count) {

    vkDestroyImageView(g_window.device, brick->image_view[lod_index], 0);
    vkDestroySampler(g_window.device, brick->sampler[lod_index], 0);

    lod_index++;
  }

  vkFreeMemory(g_window.device, brick->device_memory, 0);
  vkDestroyImage(g_window.device, brick->image, 0);
}
