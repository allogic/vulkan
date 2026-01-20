#include <pch.h>

static void renderer_create_command_buffer(void);
static void renderer_create_sync_objects(void);
static void renderer_create_descriptor_pools(void);
static void renderer_create_descriptor_set_layouts(void);
static void renderer_create_descriptor_sets(void);
static void renderer_create_pipeline_layouts(void);

static void renderer_create_debug_line_pipeline(char const *vertex_shader_file_path, char const *fragment_shader_file_path);

static void renderer_create_time_info_buffer(void);
static void renderer_create_screen_info_buffer(void);
static void renderer_create_camera_info_buffer(void);
static void renderer_create_debug_line_vertex_buffer(void);
static void renderer_create_debug_line_index_buffer(void);

static void renderer_update_debug_line_descriptor_sets(void);

static void renderer_update_uniform_buffers(transform_t *transform, camera_t *camera);

static void renderer_record_compute_commands(void);
static void renderer_record_graphics_commands(void);

static void renderer_destroy_command_buffer(void);
static void renderer_destroy_sync_objects(void);
static void renderer_destroy_descriptor_pools(void);
static void renderer_destroy_descriptor_set_layouts(void);
static void renderer_destroy_descriptor_sets(void);
static void renderer_destroy_pipeline_layouts(void);
static void renderer_destroy_pipelines(void);

static void renderer_destroy_time_info_buffer(void);
static void renderer_destroy_screen_info_buffer(void);
static void renderer_destroy_camera_info_buffer(void);
static void renderer_destroy_debug_line_vertex_buffer(void);
static void renderer_destroy_debug_line_index_buffer(void);

renderer_t g_renderer = {0};

static VkDescriptorPoolSize const s_debug_line_descriptor_pool_sizes[] = {
  {
    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 6,
  },
};

static VkDescriptorSetLayoutBinding const s_debug_line_descriptor_set_layout_bindings[] = {
  {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .pImmutableSamplers = 0,
  },
  {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .pImmutableSamplers = 0,
  },
  {
    .binding = 2,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .pImmutableSamplers = 0,
  },
};

static VkVertexInputBindingDescription const s_debug_line_vertex_input_binding_descriptions[] = {
  {
    .binding = 0,
    .stride = sizeof(debug_line_vertex_t),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  },
};

static VkVertexInputAttributeDescription const s_debug_line_vertex_input_attribute_descriptions[] = {
  {
    .location = 0,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = 0,
  },
  {
    .location = 1,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
    .offset = OFFSET_OF(debug_line_vertex_t, color),
  },
};

void renderer_create(int32_t frames_in_flight) {
  g_renderer.is_debug_enabled = 1;
  g_renderer.frames_in_flight = clampi(frames_in_flight, 1, g_swapchain.image_count);

  renderer_create_command_buffer();
  renderer_create_sync_objects();
  renderer_create_descriptor_pools();
  renderer_create_descriptor_set_layouts();
  renderer_create_descriptor_sets();
  renderer_create_pipeline_layouts();

  renderer_create_debug_line_pipeline(ROOT_DIR "/shader/debug/line.vert.spv", ROOT_DIR "/shader/debug/line.frag.spv");

  renderer_create_time_info_buffer();
  renderer_create_screen_info_buffer();
  renderer_create_camera_info_buffer();
  renderer_create_debug_line_vertex_buffer();
  renderer_create_debug_line_index_buffer();

  renderer_update_debug_line_descriptor_sets();
}
void renderer_draw(transform_t *transform, camera_t *camera) {
  VkResult result = VK_SUCCESS;

  result = vkWaitForFences(g_window.device, 1, &g_renderer.frame_fence[g_renderer.frame_index], 1, UINT64_MAX);

  switch (result) {
    case VK_SUCCESS: {
      break;
    }
#ifdef BUILD_DEBUG
    default: {
      __debugbreak();
    }
#endif // BUILD_DEBUG
  }

  result = vkResetFences(g_window.device, 1, &g_renderer.frame_fence[g_renderer.frame_index]);

  switch (result) {
    case VK_SUCCESS: {
      break;
    }
#ifdef BUILD_DEBUG
    default: {
      __debugbreak();
    }
#endif // BUILD_DEBUG
  }

  result = vkResetCommandBuffer(g_renderer.command_buffer[g_renderer.frame_index], 0);

  switch (result) {
    case VK_SUCCESS: {
      break;
    }
#ifdef BUILD_DEBUG
    default: {
      __debugbreak();
    }
#endif // BUILD_DEBUG
  }

  result = vkAcquireNextImageKHR(g_window.device, g_swapchain.handle, UINT64_MAX, g_renderer.image_available_semaphore[g_renderer.frame_index], 0, &g_renderer.image_index);

  switch (result) {
    case VK_SUCCESS: {
      break;
    }
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR: {

      g_swapchain.is_dirty = 1;

      return;
    }
#ifdef BUILD_DEBUG
    default: {
      __debugbreak();
    }
#endif // BUILD_DEBUG
  }

  renderer_update_uniform_buffers(transform, camera);

  VkCommandBufferBeginInfo command_buffer_begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = 0,
  };

  VK_CHECK(vkBeginCommandBuffer(g_renderer.command_buffer[g_renderer.frame_index], &command_buffer_begin_info));

  renderer_record_compute_commands();
  renderer_record_graphics_commands();

  VK_CHECK(vkEndCommandBuffer(g_renderer.command_buffer[g_renderer.frame_index]));

  // TODO: re-validate VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  VkPipelineStageFlags graphics_wait_stages[] = {
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };

  VkSubmitInfo graphics_submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pWaitSemaphores = &g_renderer.image_available_semaphore[g_renderer.frame_index],
    .waitSemaphoreCount = 1,
    .pSignalSemaphores = &g_renderer.render_finished_semaphore[g_renderer.image_index],
    .signalSemaphoreCount = 1,
    .pCommandBuffers = &g_renderer.command_buffer[g_renderer.frame_index],
    .commandBufferCount = 1,
    .pWaitDstStageMask = graphics_wait_stages,
  };

  result = vkQueueSubmit(g_window.primary_queue, 1, &graphics_submit_info, g_renderer.frame_fence[g_renderer.frame_index]);

  switch (result) {
    case VK_SUCCESS: {
      break;
    }
#ifdef BUILD_DEBUG
    default: {
      __debugbreak();
    }
#endif // BUILD_DEBUG
  }

  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pWaitSemaphores = &g_renderer.render_finished_semaphore[g_renderer.image_index],
    .waitSemaphoreCount = 1,
    .pSwapchains = &g_swapchain.handle,
    .swapchainCount = 1,
    .pImageIndices = &g_renderer.image_index,
  };

  result = vkQueuePresentKHR(g_window.present_queue, &present_info);

  switch (result) {
    case VK_SUCCESS: {
      break;
    }
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR: {

      g_swapchain.is_dirty = 1;

      return;
    }
#ifdef BUILD_DEBUG
    default: {
      __debugbreak();
    }
#endif // BUILD_DEBUG
  }

  g_renderer.frame_index = (g_renderer.frame_index + 1) % g_renderer.frames_in_flight;
}
void renderer_destroy(void) {
  renderer_destroy_debug_line_index_buffer();
  renderer_destroy_debug_line_vertex_buffer();
  renderer_destroy_camera_info_buffer();
  renderer_destroy_screen_info_buffer();
  renderer_destroy_time_info_buffer();

  renderer_destroy_pipelines();
  renderer_destroy_pipeline_layouts();
  renderer_destroy_descriptor_sets();
  renderer_destroy_descriptor_set_layouts();
  renderer_destroy_descriptor_pools();
  renderer_destroy_sync_objects();
  renderer_destroy_command_buffer();
}

void renderer_draw_debug_line(vector3_t from, vector3_t to, vector4_t color) {
  if (g_renderer.is_debug_enabled) {

    uint32_t vertex_offset = g_renderer.debug_line_vertex_offset[g_renderer.frame_index];
    uint32_t index_offset = g_renderer.debug_line_index_offset[g_renderer.frame_index];

    debug_line_vertex_t *vertices = g_renderer.debug_line_vertex[g_renderer.frame_index];
    debug_line_index_t *indices = g_renderer.debug_line_index[g_renderer.frame_index];

    vertices[vertex_offset + 0].position = from;
    vertices[vertex_offset + 1].position = to;

    vertices[vertex_offset + 0].color = color;
    vertices[vertex_offset + 1].color = color;

    indices[index_offset + 0] = (debug_line_index_t)(vertex_offset + 0);
    indices[index_offset + 1] = (debug_line_index_t)(vertex_offset + 1);

    g_renderer.debug_line_vertex_offset[g_renderer.frame_index] += 2;
    g_renderer.debug_line_index_offset[g_renderer.frame_index] += 2;
  }
}
void renderer_draw_debug_box(vector3_t position, vector3_t size, vector4_t color) {
  if (g_renderer.is_debug_enabled) {

    uint32_t vertex_offset = g_renderer.debug_line_vertex_offset[g_renderer.frame_index];
    uint32_t index_offset = g_renderer.debug_line_index_offset[g_renderer.frame_index];

    debug_line_vertex_t *vertices = g_renderer.debug_line_vertex[g_renderer.frame_index];
    debug_line_index_t *indices = g_renderer.debug_line_index[g_renderer.frame_index];

    vertices[vertex_offset + 0].position = (vector3_t){position.x, position.y, position.z};
    vertices[vertex_offset + 1].position = (vector3_t){position.x, position.y + size.y, position.z};
    vertices[vertex_offset + 2].position = (vector3_t){position.x + size.x, position.y, position.z};
    vertices[vertex_offset + 3].position = (vector3_t){position.x + size.x, position.y + size.y, position.z};
    vertices[vertex_offset + 4].position = (vector3_t){position.x, position.y, position.z + size.z};
    vertices[vertex_offset + 5].position = (vector3_t){position.x, position.y + size.y, position.z + size.z};
    vertices[vertex_offset + 6].position = (vector3_t){position.x + size.x, position.y, position.z + size.z};
    vertices[vertex_offset + 7].position = (vector3_t){position.x + size.x, position.y + size.y, position.z + size.z};

    vertices[vertex_offset + 0].color = color;
    vertices[vertex_offset + 1].color = color;
    vertices[vertex_offset + 2].color = color;
    vertices[vertex_offset + 3].color = color;
    vertices[vertex_offset + 4].color = color;
    vertices[vertex_offset + 5].color = color;
    vertices[vertex_offset + 6].color = color;
    vertices[vertex_offset + 7].color = color;

    indices[index_offset + 0] = (debug_line_index_t)(vertex_offset + 0);
    indices[index_offset + 1] = (debug_line_index_t)(vertex_offset + 1);
    indices[index_offset + 2] = (debug_line_index_t)(vertex_offset + 1);
    indices[index_offset + 3] = (debug_line_index_t)(vertex_offset + 3);
    indices[index_offset + 4] = (debug_line_index_t)(vertex_offset + 3);
    indices[index_offset + 5] = (debug_line_index_t)(vertex_offset + 2);
    indices[index_offset + 6] = (debug_line_index_t)(vertex_offset + 2);
    indices[index_offset + 7] = (debug_line_index_t)(vertex_offset + 0);
    indices[index_offset + 8] = (debug_line_index_t)(vertex_offset + 4);
    indices[index_offset + 9] = (debug_line_index_t)(vertex_offset + 5);
    indices[index_offset + 10] = (debug_line_index_t)(vertex_offset + 5);
    indices[index_offset + 11] = (debug_line_index_t)(vertex_offset + 7);
    indices[index_offset + 12] = (debug_line_index_t)(vertex_offset + 7);
    indices[index_offset + 13] = (debug_line_index_t)(vertex_offset + 6);
    indices[index_offset + 14] = (debug_line_index_t)(vertex_offset + 6);
    indices[index_offset + 15] = (debug_line_index_t)(vertex_offset + 4);
    indices[index_offset + 16] = (debug_line_index_t)(vertex_offset + 0);
    indices[index_offset + 17] = (debug_line_index_t)(vertex_offset + 4);
    indices[index_offset + 18] = (debug_line_index_t)(vertex_offset + 1);
    indices[index_offset + 19] = (debug_line_index_t)(vertex_offset + 5);
    indices[index_offset + 20] = (debug_line_index_t)(vertex_offset + 2);
    indices[index_offset + 21] = (debug_line_index_t)(vertex_offset + 6);
    indices[index_offset + 22] = (debug_line_index_t)(vertex_offset + 3);
    indices[index_offset + 23] = (debug_line_index_t)(vertex_offset + 7);

    g_renderer.debug_line_vertex_offset[g_renderer.frame_index] += 8;
    g_renderer.debug_line_index_offset[g_renderer.frame_index] += 24;
  }
}

static void renderer_create_command_buffer(void) {
  VkCommandBufferAllocateInfo command_buffer_alloc_create_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = g_window.command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = g_renderer.frames_in_flight,
  };

  VK_CHECK(vkAllocateCommandBuffers(g_window.device, &command_buffer_alloc_create_info, g_renderer.command_buffer));
}
static void renderer_create_sync_objects(void) {
  VkSemaphoreCreateInfo semaphore_create_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .flags = 0,
  };

  VkFenceCreateInfo fence_create_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  int32_t image_index = 0;
  int32_t image_count = g_swapchain.image_count;

  while (image_index < image_count) {

    VK_CHECK(vkCreateSemaphore(g_window.device, &semaphore_create_info, 0, &g_renderer.render_finished_semaphore[image_index]));

    image_index++;
  }

  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    VK_CHECK(vkCreateSemaphore(g_window.device, &semaphore_create_info, 0, &g_renderer.image_available_semaphore[frame_index]));
    VK_CHECK(vkCreateFence(g_window.device, &fence_create_info, 0, &g_renderer.frame_fence[frame_index]));

    frame_index++;
  }
}
static void renderer_create_descriptor_pools(void) {
  {
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pPoolSizes = s_debug_line_descriptor_pool_sizes,
      .poolSizeCount = ARRAY_COUNT(s_debug_line_descriptor_pool_sizes),
      .maxSets = g_renderer.frames_in_flight,
    };

    VK_CHECK(vkCreateDescriptorPool(g_window.device, &descriptor_pool_create_info, 0, &g_renderer.debug_line_descriptor_pool));
  }
}
static void renderer_create_descriptor_set_layouts(void) {
  {
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pBindings = s_debug_line_descriptor_set_layout_bindings,
      .bindingCount = ARRAY_COUNT(s_debug_line_descriptor_set_layout_bindings),
      .pNext = 0,
    };

    VK_CHECK(vkCreateDescriptorSetLayout(g_window.device, &descriptor_set_layout_create_info, 0, &g_renderer.debug_line_descriptor_set_layout));
  }
}
static void renderer_create_descriptor_sets(void) {
  {
    int32_t descriptor_set_index = 0;
    int32_t descriptor_set_count = g_renderer.frames_in_flight;

    g_renderer.debug_line_descriptor_set = (VkDescriptorSet *)HEAP_ALLOC(sizeof(VkDescriptorSet) * descriptor_set_count, 0, 0);

    VkDescriptorSetLayout *descriptor_set_layouts = (VkDescriptorSetLayout *)HEAP_ALLOC(sizeof(VkDescriptorSetLayout) * descriptor_set_count, 0, 0);

    while (descriptor_set_index < descriptor_set_count) {

      descriptor_set_layouts[descriptor_set_index] = g_renderer.debug_line_descriptor_set_layout;

      descriptor_set_index++;
    }

    VkDescriptorSetAllocateInfo debug_line_descriptor_set_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorSetCount = descriptor_set_count,
      .descriptorPool = g_renderer.debug_line_descriptor_pool,
      .pSetLayouts = descriptor_set_layouts,
    };

    VK_CHECK(vkAllocateDescriptorSets(g_window.device, &debug_line_descriptor_set_allocate_info, g_renderer.debug_line_descriptor_set));

    HEAP_FREE(descriptor_set_layouts);
  }
}
static void renderer_create_pipeline_layouts(void) {
  {
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &g_renderer.debug_line_descriptor_set_layout,
      .pPushConstantRanges = 0,
      .pushConstantRangeCount = 0,
    };

    VK_CHECK(vkCreatePipelineLayout(g_window.device, &pipeline_layout_create_info, 0, &g_renderer.debug_line_pipeline_layout));
  }
}

static void renderer_create_debug_line_pipeline(char const *vertex_shader_file_path, char const *fragment_shader_file_path) {
  VkShaderModule vertex_module = 0;
  VkShaderModule fragment_module = 0;

  {
    uint8_t *shader_bytes = 0;
    uint64_t shader_size = 0;

    fsutils_load_binary(&shader_bytes, &shader_size, vertex_shader_file_path);

    VkShaderModuleCreateInfo shader_module_create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pCode = (uint32_t const *)shader_bytes,
      .codeSize = shader_size,
    };

    VK_CHECK(vkCreateShaderModule(g_window.device, &shader_module_create_info, 0, &vertex_module));

    HEAP_FREE(shader_bytes);
  }

  {
    uint8_t *shader_bytes = 0;
    uint64_t shader_size = 0;

    fsutils_load_binary(&shader_bytes, &shader_size, fragment_shader_file_path);

    VkShaderModuleCreateInfo shader_module_create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pCode = (uint32_t const *)shader_bytes,
      .codeSize = shader_size,
    };

    VK_CHECK(vkCreateShaderModule(g_window.device, &shader_module_create_info, 0, &fragment_module));

    HEAP_FREE(shader_bytes);
  }

  VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertex_module,
    .pName = "main",
  };

  VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragment_module,
    .pName = "main",
  };

  VkPipelineShaderStageCreateInfo shader_stages[] = {
    vertex_shader_stage_create_info,
    fragment_shader_stage_create_info,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pVertexBindingDescriptions = s_debug_line_vertex_input_binding_descriptions,
    .vertexBindingDescriptionCount = ARRAY_COUNT(s_debug_line_vertex_input_binding_descriptions),
    .pVertexAttributeDescriptions = s_debug_line_vertex_input_attribute_descriptions,
    .vertexAttributeDescriptionCount = ARRAY_COUNT(s_debug_line_vertex_input_attribute_descriptions),
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    .primitiveRestartEnable = 0,
  };

  VkViewport viewport = {
    .x = 0.0F,
    .y = 0.0F,
    .width = (float)g_window.window_width,
    .height = (float)g_window.window_height,
    .minDepth = 0.0F,
    .maxDepth = 1.0F,
  };

  VkRect2D scissor = {
    .offset.x = 0,
    .offset.y = 0,
    .extent = {
      .width = g_window.window_width,
      .height = g_window.window_height,
    },
  };

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = 0,
    .rasterizerDiscardEnable = 0,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0F,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable = 0,
    .depthBiasConstantFactor = 0.0F,
    .depthBiasClamp = 0.0F,
    .depthBiasSlopeFactor = 0.0F,
  };

  VkPipelineMultisampleStateCreateInfo multisampling_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = 0,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0F,
    .pSampleMask = 0,
    .alphaToCoverageEnable = 0,
    .alphaToOneEnable = 0,
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = 1,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .alphaBlendOp = VK_BLEND_OP_ADD,
  };

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = 0,
    .depthWriteEnable = 0,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = 0,
    .stencilTestEnable = 0,
  };

  VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = 0,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment,
    .blendConstants = {
      0.0F,
      0.0F,
      0.0F,
      0.0F,
    },
  };

  VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .pDynamicStates = dynamic_states,
    .dynamicStateCount = ARRAY_COUNT(dynamic_states),
  };

  VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pStages = shader_stages,
    .stageCount = ARRAY_COUNT(shader_stages),
    .pVertexInputState = &vertex_input_create_info,
    .pInputAssemblyState = &input_assembly_create_info,
    .pViewportState = &viewport_state_create_info,
    .pRasterizationState = &rasterizer_create_info,
    .pMultisampleState = &multisampling_create_info,
    .pDepthStencilState = &depth_stencil_state_create_info,
    .pColorBlendState = &color_blend_create_info,
    .pDynamicState = &dynamic_state_create_info,
    .layout = g_renderer.debug_line_pipeline_layout,
    .renderPass = g_renderpass_main,
    .subpass = 0,
    .basePipelineHandle = 0,
  };

  VK_CHECK(vkCreateGraphicsPipelines(g_window.device, 0, 1, &graphics_pipeline_create_info, 0, &g_renderer.debug_line_pipeline));

  vkDestroyShaderModule(g_window.device, vertex_module, 0);
  vkDestroyShaderModule(g_window.device, fragment_module, 0);
}

static void renderer_create_time_info_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    uint64_t buffer_size = sizeof(time_info_t);

    VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateBuffer(g_window.device, &buffer_create_info, 0, &g_renderer.time_info_buffer[frame_index]));

    VkMemoryRequirements memory_requirements = {0};

    vkGetBufferMemoryRequirements(g_window.device, g_renderer.time_info_buffer[frame_index], &memory_requirements);

    int32_t memory_type_index = vktuils_find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &g_renderer.time_info_buffer_device_memory[frame_index]));
    VK_CHECK(vkBindBufferMemory(g_window.device, g_renderer.time_info_buffer[frame_index], g_renderer.time_info_buffer_device_memory[frame_index], 0));
    VK_CHECK(vkMapMemory(g_window.device, g_renderer.time_info_buffer_device_memory[frame_index], 0, buffer_size, 0, &g_renderer.time_info[frame_index]));

    frame_index++;
  }
}
static void renderer_create_screen_info_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    uint64_t buffer_size = sizeof(screen_info_t);

    VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateBuffer(g_window.device, &buffer_create_info, 0, &g_renderer.screen_info_buffer[frame_index]));

    VkMemoryRequirements memory_requirements = {0};

    vkGetBufferMemoryRequirements(g_window.device, g_renderer.screen_info_buffer[frame_index], &memory_requirements);

    int32_t memory_type_index = vktuils_find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &g_renderer.screen_info_buffer_device_memory[frame_index]));
    VK_CHECK(vkBindBufferMemory(g_window.device, g_renderer.screen_info_buffer[frame_index], g_renderer.screen_info_buffer_device_memory[frame_index], 0));
    VK_CHECK(vkMapMemory(g_window.device, g_renderer.screen_info_buffer_device_memory[frame_index], 0, buffer_size, 0, &g_renderer.screen_info[frame_index]));

    frame_index++;
  }
}
static void renderer_create_camera_info_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    uint64_t buffer_size = sizeof(camera_info_t);

    VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateBuffer(g_window.device, &buffer_create_info, 0, &g_renderer.camera_info_buffer[frame_index]));

    VkMemoryRequirements memory_requirements = {0};

    vkGetBufferMemoryRequirements(g_window.device, g_renderer.camera_info_buffer[frame_index], &memory_requirements);

    int32_t memory_type_index = vktuils_find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &g_renderer.camera_info_buffer_device_memory[frame_index]));
    VK_CHECK(vkBindBufferMemory(g_window.device, g_renderer.camera_info_buffer[frame_index], g_renderer.camera_info_buffer_device_memory[frame_index], 0));
    VK_CHECK(vkMapMemory(g_window.device, g_renderer.camera_info_buffer_device_memory[frame_index], 0, buffer_size, 0, &g_renderer.camera_info[frame_index]));

    frame_index++;
  }
}
static void renderer_create_debug_line_vertex_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    uint64_t buffer_size = sizeof(debug_line_vertex_t) * RENDERER_DEBUG_LINE_VERTEX_COUNT;

    VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateBuffer(g_window.device, &buffer_create_info, 0, &g_renderer.debug_line_vertex_buffer[frame_index]));

    VkMemoryRequirements memory_requirements = {0};

    vkGetBufferMemoryRequirements(g_window.device, g_renderer.debug_line_vertex_buffer[frame_index], &memory_requirements);

    int32_t memory_type_index = vktuils_find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &g_renderer.debug_line_vertex_buffer_device_memory[frame_index]));
    VK_CHECK(vkBindBufferMemory(g_window.device, g_renderer.debug_line_vertex_buffer[frame_index], g_renderer.debug_line_vertex_buffer_device_memory[frame_index], 0));
    VK_CHECK(vkMapMemory(g_window.device, g_renderer.debug_line_vertex_buffer_device_memory[frame_index], 0, buffer_size, 0, &g_renderer.debug_line_vertex[frame_index]));

    frame_index++;
  }
}
static void renderer_create_debug_line_index_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    uint64_t buffer_size = sizeof(debug_line_index_t) * RENDERER_DEBUG_LINE_INDEX_COUNT;

    VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateBuffer(g_window.device, &buffer_create_info, 0, &g_renderer.debug_line_index_buffer[frame_index]));

    VkMemoryRequirements memory_requirements = {0};

    vkGetBufferMemoryRequirements(g_window.device, g_renderer.debug_line_index_buffer[frame_index], &memory_requirements);

    int32_t memory_type_index = vktuils_find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    VK_CHECK(vkAllocateMemory(g_window.device, &memory_allocate_info, 0, &g_renderer.debug_line_index_buffer_device_memory[frame_index]));
    VK_CHECK(vkBindBufferMemory(g_window.device, g_renderer.debug_line_index_buffer[frame_index], g_renderer.debug_line_index_buffer_device_memory[frame_index], 0));
    VK_CHECK(vkMapMemory(g_window.device, g_renderer.debug_line_index_buffer_device_memory[frame_index], 0, buffer_size, 0, &g_renderer.debug_line_index[frame_index]));

    frame_index++;
  }
}

static void renderer_update_debug_line_descriptor_sets(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    VkDescriptorBufferInfo time_descriptor_buffer_infos[] = {
      {
        .offset = 0,
        .buffer = g_renderer.time_info_buffer[frame_index],
        .range = VK_WHOLE_SIZE,
      },
    };

    VkDescriptorBufferInfo screen_descriptor_buffer_infos[] = {
      {
        .offset = 0,
        .buffer = g_renderer.screen_info_buffer[frame_index],
        .range = VK_WHOLE_SIZE,
      },
    };

    VkDescriptorBufferInfo camera_descriptor_buffer_infos[] = {
      {
        .offset = 0,
        .buffer = g_renderer.camera_info_buffer[frame_index],
        .range = VK_WHOLE_SIZE,
      },
    };

    VkWriteDescriptorSet write_descriptor_sets[] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = 0,
        .dstSet = g_renderer.debug_line_descriptor_set[frame_index],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = ARRAY_COUNT(time_descriptor_buffer_infos),
        .pImageInfo = 0,
        .pBufferInfo = time_descriptor_buffer_infos,
        .pTexelBufferView = 0,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = 0,
        .dstSet = g_renderer.debug_line_descriptor_set[frame_index],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = ARRAY_COUNT(screen_descriptor_buffer_infos),
        .pImageInfo = 0,
        .pBufferInfo = screen_descriptor_buffer_infos,
        .pTexelBufferView = 0,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = 0,
        .dstSet = g_renderer.debug_line_descriptor_set[frame_index],
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = ARRAY_COUNT(camera_descriptor_buffer_infos),
        .pImageInfo = 0,
        .pBufferInfo = camera_descriptor_buffer_infos,
        .pTexelBufferView = 0,
      },
    };

    vkUpdateDescriptorSets(g_window.device, ARRAY_COUNT(write_descriptor_sets), write_descriptor_sets, 0, 0);

    frame_index++;
  }
}

static void renderer_update_uniform_buffers(transform_t *transform, camera_t *camera) {
  transform_compute_world_position(transform);
  transform_compute_world_rotation(transform);
  transform_compute_world_scale(transform);

  float window_width = (float)g_window.window_width;
  float window_height = (float)g_window.window_height;

  vector3_t eye = transform->world_position;
  vector3_t center = vector3_add(transform->world_position, transform_local_front(transform));
  vector3_t up = vector3_down();

  float fov = deg_to_rad(camera->fov);
  float aspect_ratio = window_width / window_height;
  float near_z = camera->near_z;
  float far_z = camera->far_z;

  matrix4_t view = matrix4_look_at(eye, center, up);
  matrix4_t projection = matrix4_persp(fov, aspect_ratio, near_z, far_z);
  matrix4_t view_projection = matrix4_mul(view, projection);
  matrix4_t view_projection_inv = matrix4_inverse(view_projection);

  g_renderer.time_info[g_renderer.frame_index]->time = g_window.time;
  g_renderer.time_info[g_renderer.frame_index]->delta_time = g_window.delta_time;

  g_renderer.screen_info[g_renderer.frame_index]->resolution = vector2_xy(window_width, window_height);

  g_renderer.camera_info[g_renderer.frame_index]->position = transform->world_position;
  g_renderer.camera_info[g_renderer.frame_index]->view = view;
  g_renderer.camera_info[g_renderer.frame_index]->projection = projection;
  g_renderer.camera_info[g_renderer.frame_index]->view_projection = view_projection;
  g_renderer.camera_info[g_renderer.frame_index]->view_projection_inv = view_projection_inv;
}

static void renderer_record_compute_commands(void) {
  // TODO
}
static void renderer_record_graphics_commands(void) {
  VkClearValue color_clear_value = {
    .color.float32 = {
      0.0F,
      0.0F,
      0.0F,
      1.0F,
    },
  };

  VkClearValue depth_clear_value = {
    .depthStencil = {
      .depth = 1.0F,
      .stencil = 0,
    },
  };

  VkClearValue clear_values[] = {
    color_clear_value,
    depth_clear_value,
  };

  VkRenderPassBeginInfo render_pass_create_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = g_renderpass_main,
    .framebuffer = g_swapchain.frame_buffer[g_renderer.image_index],
    .renderArea = {
      .offset.x = 0,
      .offset.y = 0,
      .extent = {
        .width = g_window.window_width,
        .height = g_window.window_height,
      },
    },
    .pClearValues = clear_values,
    .clearValueCount = ARRAY_COUNT(clear_values),
  };

  vkCmdBeginRenderPass(g_renderer.command_buffer[g_renderer.frame_index], &render_pass_create_info, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {
    .x = 0.0F,
    .y = 0.0F,
    .width = (float)g_window.window_width,
    .height = (float)g_window.window_height,
    .minDepth = 0.0F,
    .maxDepth = 1.0F,
  };

  vkCmdSetViewport(g_renderer.command_buffer[g_renderer.frame_index], 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = g_window.window_width;
  scissor.extent.height = g_window.window_height;

  vkCmdSetScissor(g_renderer.command_buffer[g_renderer.frame_index], 0, 1, &scissor);

  {
    // VkBuffer vertex_buffers[] = {s_renderer_cluster_vertex_buffer[s_renderer_frame_index]};
    // uint64_t vertex_offsets[] = {0, 0};
    //
    // vkCmdBindPipeline(s_renderer_graphics_command_buffers[s_renderer_frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, s_renderer_chunk_renderer_pipeline);
    // vkCmdBindVertexBuffers(s_renderer_graphics_command_buffers[s_renderer_frame_index], 0, CORE_ARRAY_COUNT(vertex_buffers), vertex_buffers, vertex_offsets);
    // vkCmdBindIndexBuffer(s_renderer_graphics_command_buffers[s_renderer_frame_index], s_renderer_cluster_index_buffer[s_renderer_frame_index], 0, VK_INDEX_TYPE_UINT32);
    // vkCmdBindDescriptorSets(s_renderer_graphics_command_buffers[s_renderer_frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, s_renderer_chunk_renderer_pipeline_layout, 0, 1, &s_renderer_chunk_renderer_descriptor_sets[s_renderer_frame_index], 0, 0);
    // vkCmdDrawIndexed(s_renderer_graphics_command_buffers[s_renderer_frame_index], RENDERER_CLUSTER_INDEX_COUNT, 1, 0, 0, 0);
  }

  {
    if (g_renderer.is_debug_enabled) {

      VkBuffer vertex_buffers[] = {g_renderer.debug_line_vertex_buffer[g_renderer.frame_index]};
      uint64_t vertex_offsets[] = {0};

      vkCmdBindPipeline(g_renderer.command_buffer[g_renderer.frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, g_renderer.debug_line_pipeline);
      vkCmdBindVertexBuffers(g_renderer.command_buffer[g_renderer.frame_index], 0, ARRAY_COUNT(vertex_buffers), vertex_buffers, vertex_offsets);
      vkCmdBindIndexBuffer(g_renderer.command_buffer[g_renderer.frame_index], g_renderer.debug_line_index_buffer[g_renderer.frame_index], 0, VK_INDEX_TYPE_UINT32);
      vkCmdBindDescriptorSets(g_renderer.command_buffer[g_renderer.frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, g_renderer.debug_line_pipeline_layout, 0, 1, &g_renderer.debug_line_descriptor_set[g_renderer.frame_index], 0, 0);
      vkCmdDrawIndexed(g_renderer.command_buffer[g_renderer.frame_index], g_renderer.debug_line_index_offset[g_renderer.frame_index], 1, 0, 0, 0);

      g_renderer.debug_line_vertex_offset[g_renderer.frame_index] = 0;
      g_renderer.debug_line_index_offset[g_renderer.frame_index] = 0;
    }
  }

  vkCmdEndRenderPass(g_renderer.command_buffer[g_renderer.frame_index]);
}

static void renderer_destroy_command_buffer(void) {
  vkFreeCommandBuffers(g_window.device, g_window.command_pool, g_renderer.frames_in_flight, g_renderer.command_buffer);
}
static void renderer_destroy_sync_objects(void) {
  int32_t image_index = 0;
  int32_t image_count = g_swapchain.image_count;

  while (image_index < image_count) {

    vkDestroySemaphore(g_window.device, g_renderer.render_finished_semaphore[image_index], 0);

    image_index++;
  }

  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    vkDestroySemaphore(g_window.device, g_renderer.image_available_semaphore[frame_index], 0);
    vkDestroyFence(g_window.device, g_renderer.frame_fence[frame_index], 0);

    frame_index++;
  }
}
static void renderer_destroy_descriptor_pools(void) {
  vkDestroyDescriptorPool(g_window.device, g_renderer.debug_line_descriptor_pool, 0);
}
static void renderer_destroy_descriptor_set_layouts(void) {
  vkDestroyDescriptorSetLayout(g_window.device, g_renderer.debug_line_descriptor_set_layout, 0);
}
static void renderer_destroy_descriptor_sets(void) {
  HEAP_FREE(g_renderer.debug_line_descriptor_set);
}
static void renderer_destroy_pipeline_layouts(void) {
  vkDestroyPipelineLayout(g_window.device, g_renderer.debug_line_pipeline_layout, 0);
}
static void renderer_destroy_pipelines(void) {
  vkDestroyPipeline(g_window.device, g_renderer.debug_line_pipeline, 0);
}

static void renderer_destroy_time_info_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    vkUnmapMemory(g_window.device, g_renderer.time_info_buffer_device_memory[frame_index]);
    vkFreeMemory(g_window.device, g_renderer.time_info_buffer_device_memory[frame_index], 0);
    vkDestroyBuffer(g_window.device, g_renderer.time_info_buffer[frame_index], 0);

    frame_index++;
  }
}
static void renderer_destroy_screen_info_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    vkUnmapMemory(g_window.device, g_renderer.screen_info_buffer_device_memory[frame_index]);
    vkFreeMemory(g_window.device, g_renderer.screen_info_buffer_device_memory[frame_index], 0);
    vkDestroyBuffer(g_window.device, g_renderer.screen_info_buffer[frame_index], 0);

    frame_index++;
  }
}
static void renderer_destroy_camera_info_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    vkUnmapMemory(g_window.device, g_renderer.camera_info_buffer_device_memory[frame_index]);
    vkFreeMemory(g_window.device, g_renderer.camera_info_buffer_device_memory[frame_index], 0);
    vkDestroyBuffer(g_window.device, g_renderer.camera_info_buffer[frame_index], 0);

    frame_index++;
  }
}
static void renderer_destroy_debug_line_vertex_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    vkUnmapMemory(g_window.device, g_renderer.debug_line_vertex_buffer_device_memory[frame_index]);
    vkFreeMemory(g_window.device, g_renderer.debug_line_vertex_buffer_device_memory[frame_index], 0);
    vkDestroyBuffer(g_window.device, g_renderer.debug_line_vertex_buffer[frame_index], 0);

    frame_index++;
  }
}
static void renderer_destroy_debug_line_index_buffer(void) {
  int32_t frame_index = 0;
  int32_t frame_count = g_renderer.frames_in_flight;

  while (frame_index < frame_count) {

    vkUnmapMemory(g_window.device, g_renderer.debug_line_index_buffer_device_memory[frame_index]);
    vkFreeMemory(g_window.device, g_renderer.debug_line_index_buffer_device_memory[frame_index], 0);
    vkDestroyBuffer(g_window.device, g_renderer.debug_line_index_buffer[frame_index], 0);

    frame_index++;
  }
}
