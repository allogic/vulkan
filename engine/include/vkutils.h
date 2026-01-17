#ifndef VKUTILS_H
#define VKUTILS_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int32_t vktuils_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags memory_property_flags);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // VKUTILS_H
