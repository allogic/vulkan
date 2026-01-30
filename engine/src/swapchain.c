#include <pch.h>

swapchain_t g_swapchain = {0};

static void swapchain_create_color_images(void);
static void swapchain_create_depth_images(void);
static void swapchain_create_frame_buffer(void);

static void swapchain_destroy_color_images(void);
static void swapchain_destroy_depth_images(void);
static void swapchain_destroy_frame_buffer(void);

void swapchain_create(int32_t image_count) {
  g_swapchain.image_count = clampi(image_count, g_window.min_image_count, g_window.max_image_count);

  int32_t queue_families[2] = {
    g_window.primary_queue_index,
    g_window.present_queue_index,
  };

  VkSwapchainCreateInfoKHR swapchain_create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = g_window.surface,
    .minImageCount = g_swapchain.image_count,
    .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
    .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    .imageExtent.width = g_window.window_width,
    .imageExtent.height = g_window.window_height,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = g_window.surface_transform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
    .clipped = 1,
    .oldSwapchain = 0,
    .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
    .pQueueFamilyIndices = queue_families,
    .queueFamilyIndexCount = ARRAY_COUNT(queue_families),
  };

  VK_CHECK(vkCreateSwapchainKHR(g_window.device, &swapchain_create_info, 0, &g_swapchain.handle));

  swapchain_create_color_images();
  swapchain_create_depth_images();
  swapchain_create_frame_buffer();
}
void swapchain_destroy(void) {
  swapchain_destroy_frame_buffer();
  swapchain_destroy_depth_images();
  swapchain_destroy_color_images();

  vkDestroySwapchainKHR(g_window.device, g_swapchain.handle, 0);
}

static void swapchain_create_color_images(void) {
  VK_CHECK(vkGetSwapchainImagesKHR(g_window.device, g_swapchain.handle, (uint32_t *)&g_swapchain.image_count, g_swapchain.color_image));

  uint32_t image_index = 0;
  uint32_t image_count = g_swapchain.image_count;

  while (image_index < image_count) {

    VkImageViewCreateInfo image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = g_swapchain.color_image[image_index],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_B8G8R8A8_UNORM,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
    };

    VK_CHECK(vkCreateImageView(g_window.device, &image_view_create_info, 0, &g_swapchain.color_image_view[image_index]));

    image_index++;
  }
}
static void swapchain_create_depth_images(void) {
  uint32_t image_index = 0;
  uint32_t image_count = g_swapchain.image_count;

  while (image_index < image_count) {

    VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent.width = g_window.window_width,
      .extent.height = g_window.window_height,
      .extent.depth = 1,
      .mipLevels = 1,
      .arrayLayers = 1,
      .format = VK_FORMAT_D32_SFLOAT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateImage(g_window.device, &image_create_info, 0, &g_swapchain.depth_image[image_index]));

    VkMemoryRequirements memory_requirements = {0};

    vkGetImageMemoryRequirements(g_window.device, g_swapchain.depth_image[image_index], &memory_requirements);

    uint32_t memory_type_index = vkutils_find_memory_type_index(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &g_swapchain.depth_device_memory[image_index]));
    VK_CHECK(vkBindImageMemory(g_window.device, g_swapchain.depth_image[image_index], g_swapchain.depth_device_memory[image_index], 0));

    VkImageViewCreateInfo image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = g_swapchain.depth_image[image_index],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_D32_SFLOAT,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
    };

    VK_CHECK(vkCreateImageView(g_window.device, &image_view_create_info, 0, &g_swapchain.depth_image_view[image_index]));

    image_index++;
  }
}
static void swapchain_create_frame_buffer(void) {
  uint32_t image_index = 0;
  uint32_t image_count = g_swapchain.image_count;

  while (image_index < image_count) {

    VkImageView image_attachments[] = {
      g_swapchain.color_image_view[image_index],
      g_swapchain.depth_image_view[image_index],
    };

    VkFramebufferCreateInfo frame_buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = g_renderpass_main,
      .pAttachments = image_attachments,
      .attachmentCount = ARRAY_COUNT(image_attachments),
      .width = g_window.window_width,
      .height = g_window.window_height,
      .layers = 1,
    };

    VK_CHECK(vkCreateFramebuffer(g_window.device, &frame_buffer_create_info, 0, &g_swapchain.frame_buffer[image_index]));

    image_index++;
  }
}

static void swapchain_destroy_color_images(void) {
  uint32_t image_index = 0;
  uint32_t image_count = g_swapchain.image_count;

  while (image_index < image_count) {

    vkDestroyImageView(g_window.device, g_swapchain.color_image_view[image_index], 0);

    image_index++;
  }
}
static void swapchain_destroy_depth_images(void) {
  uint32_t image_index = 0;
  uint32_t image_count = g_swapchain.image_count;

  while (image_index < image_count) {

    vkDestroyImageView(g_window.device, g_swapchain.depth_image_view[image_index], 0);
    vkFreeMemory(g_window.device, g_swapchain.depth_device_memory[image_index], 0);
    vkDestroyImage(g_window.device, g_swapchain.depth_image[image_index], 0);

    image_index++;
  }
}
static void swapchain_destroy_frame_buffer(void) {
  uint32_t image_index = 0;
  uint32_t image_count = g_swapchain.image_count;

  while (image_index < image_count) {

    vkDestroyFramebuffer(g_window.device, g_swapchain.frame_buffer[image_index], 0);

    image_index++;
  }
}
