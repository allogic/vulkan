#include <pch.h>

static vdb_brick_t vdb_create_brick(void);

vdb_t g_vdb = {0};

void vdb_create(void) {
  ivector3_t cluster_dim = {VDB_CLUSTER_DIM_X, VDB_CLUSTER_DIM_Y, VDB_CLUSTER_DIM_Z};

  vdb_info_t vdb_info = {
    .cluster_dim = cluster_dim,
  };

  g_vdb.info_buffer = buffer_create_uniform(&vdb_info, sizeof(vdb_info_t));
  g_vdb.layer_buffer = buffer_create_uniform_coherent(0, sizeof(vdb_layer_t) * VDB_MAX_LAYER);

  for (int32_t z = 0; z < VDB_CLUSTER_DIM_Z; z++) {
    for (int32_t y = 0; y < VDB_CLUSTER_DIM_Y; y++) {
      for (int32_t x = 0; x < VDB_CLUSTER_DIM_X; x++) {
        ivector3_t brick_position = {x, y, z};

        int32_t brick_index = vec_to_index(brick_position, cluster_dim);

        // g_vdb.brick[brick_index] = image_create();
      }
    }
  }
}
void vdb_destroy(void) {
  buffer_destroy(&g_vdb.info_buffer);
  buffer_destroy(&g_vdb.layer_buffer);
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

  VkImageViewCreateInfo image_view_create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = brick.image,
    .viewType = VK_IMAGE_VIEW_TYPE_3D,
    .format = VK_FORMAT_R32_UINT,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = VDB_MAX_LOD,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };

  VK_CHECK(vkCreateImageView(g_window.device, &image_view_create_info, 0, &brick.image_view));

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

  VK_CHECK(vkCreateSampler(g_window.device, &sampler_create_info, 0, &brick.sampler));

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
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
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
