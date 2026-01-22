#include <pch.h>

image_t image_create(uint32_t width, uint32_t height, uint32_t depth, uint32_t channels, VkImageType type, VkImageViewType view_type, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkImageAspectFlags aspect_flags, VkFormat format, VkImageTiling tiling, VkFilter filter) {
  image_t image = {
    .size = width * height * depth * channels,
    .width = width,
    .height = height,
    .depth = depth,
    .channels = channels,
    .usage = usage,
    .aspect_flags = aspect_flags,
    .type = type,
    .view_type = view_type,
    .format = format,
    .tiling = tiling,
    .filter = filter,
    .memory_properties = memory_properties,
  };

  VkImageCreateInfo image_create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = image.type,
    .extent.width = image.width,
    .extent.height = image.height,
    .extent.depth = image.depth,
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = image.format,
    .tiling = image.tiling,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .usage = image.usage,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  VK_CHECK(vkCreateImage(g_window.device, &image_create_info, 0, &image.handle));

  if (image.memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ||
      image.memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {

    vkGetImageMemoryRequirements(g_window.device, image.handle, &image.memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = image.memory_requirements.size,
      .memoryTypeIndex = vkutils_find_memory_type_index(image.memory_requirements.memoryTypeBits, image.memory_properties),
    };

    VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &image.device_memory));
    VK_CHECK(vkBindImageMemory(g_window.device, image.handle, image.device_memory, 0));
  }

  return image;
}
void image_destroy(image_t *image) {
  if (image->memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ||
      image->memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {

    vkUnmapMemory(g_window.device, image->device_memory);
    vkFreeMemory(g_window.device, image->device_memory, 0);
  }

  vkDestroyImage(g_window.device, image->handle, 0);
}

VkImageView image_create_view(VkImage image, VkImageViewType view_type, VkImageAspectFlags aspect_flags, VkFormat format) {
  VkImageView image_view = 0;

  VkImageViewCreateInfo image_view_create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = image,
    .viewType = view_type,
    .format = format,
    .subresourceRange = {
      .aspectMask = aspect_flags,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };

  VK_CHECK(vkCreateImageView(g_window.device, &image_view_create_info, 0, &image_view));

  return image_view;
}
VkSampler image_create_sampler(VkImage image, VkFilter filter) {
  VkSampler sampler = 0;

  VkSamplerCreateInfo sampler_create_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = filter,
    .minFilter = filter,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable = 1,
    .maxAnisotropy = g_window.max_sampler_anisotropy,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = 0,
    .compareEnable = 0,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .mipLodBias = 0.0F,
    .minLod = 0.0F,
    .maxLod = 0.0F,
  };

  VK_CHECK(vkCreateSampler(g_window.device, &sampler_create_info, 0, &sampler));

  return sampler;
}

/*
image_t image_create_2d(void *buffer, uint32_t width, uint32_t height, uint32_t channels, VkFormat format, VkImageTiling tiling, VkFilter filter) {
  buffer_t staging_buffer = buffer_create(width * height, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  image_t target_image = image_create(width, height, 1, channels, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, format, tiling, filter);

  memcpy(staging_buffer.mapped_memory, buffer, width * height);

  VkCommandBuffer command_buffer = vkutils_begin_command_buffer();
  // targetImage->LayoutTransition(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  buffer_copy_to_image(&staging_buffer, &target_image, command_buffer);
  // targetImage->LayoutTransition(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
  vkutils_end_command_buffer(command_buffer);

  return target_image;
}
image_t image_create_2d_depth_stencil(uint32_t width, uint32_t height, uint32_t channels, VkFormat format, VkImageTiling tiling, VkFilter filter) {
  return image_create(width, height, 1, channels, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, format, tiling, filter);
}
*/
