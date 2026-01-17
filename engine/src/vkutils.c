#include <pch.h>

int32_t vktuils_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags memory_property_flags) {
  int32_t memory_type = -1;

  uint32_t memory_type_index = 0;
  uint32_t memory_type_count = g_window.physical_device_memory_properties.memoryTypeCount;

  while (memory_type_index < memory_type_count) {

    if ((type_filter & (1 << memory_type_index)) && ((g_window.physical_device_memory_properties.memoryTypes[memory_type_index].propertyFlags & memory_property_flags) == memory_property_flags)) {

      memory_type = memory_type_index;

      break;
    }

    memory_type_index++;
  }

  return memory_type;
}
