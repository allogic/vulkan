#ifndef IMAGE_H
#define IMAGE_H

typedef struct image_t {
  uint64_t size;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  uint32_t channels;
  VkImageUsageFlags usage;
  VkImageAspectFlags aspect_flags;
  VkImageType type;
  VkImageViewType view_type;
  VkFormat format;
  VkImageTiling tiling;
  VkFilter filter;
  VkMemoryPropertyFlags memory_properties;
  VkMemoryRequirements memory_requirements;
  VkImage handle;
  VkDeviceMemory device_memory;
  void *mapped_memory;
} image_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

image_t image_create(uint32_t width, uint32_t height, uint32_t depth, uint32_t channels, VkImageType type, VkImageViewType view_type, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkImageAspectFlags aspect_flags, VkFormat format, VkImageTiling tiling, VkFilter filter);
void image_destroy(image_t *image);

VkImageView image_create_view(VkImage image, VkImageViewType view_type, VkImageAspectFlags aspect_flags, VkFormat format);
VkSampler image_create_sampler(VkImage image, VkFilter filter);

// image_t image_create_2d(void *buffer, uint32_t width, uint32_t height, uint32_t channels, VkFormat format, VkImageTiling tiling, VkFilter filter);
// image_t image_create_2d_depth_stencil(uint32_t width, uint32_t height, uint32_t channels, VkFormat format, VkImageTiling tiling, VkFilter filter);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // IMAGE_H
