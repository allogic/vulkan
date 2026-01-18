#ifndef MACROS_H
#define MACROS_H

#define STATIC_ASSERT(EXPRESSION) typedef uint8_t static_assert_t[(EXPRESSION) ? (1) : (-1)]

#define ARRAY_COUNT(ARRAY) ((uint64_t)(sizeof(ARRAY) / sizeof((ARRAY)[0ULL])))
#define OFFSET_OF(TYPE, MEMBER) ((uint64_t)&(((TYPE *)0)->MEMBER))

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define CLAMP(A, LO, HI) ((A) < (LO) ? (LO) : ((A) > (HI) ? (HI) : (A)))

#ifdef BUILD_DEBUG
#  define VK_CHECK(EXPRESSION)                              \
    {                                                       \
      VkResult result = (EXPRESSION);                       \
      if (result != VK_SUCCESS) {                           \
        printf("%s failed with %d\n", #EXPRESSION, result); \
        __debugbreak();                                     \
      }                                                     \
    }
#else
#  define VK_CHECK(EXPRESSION) (EXPRESSION)
#endif // BUILD_DEBUG

#endif // MACROS_H
