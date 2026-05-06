#pragma once

#if defined(__EMSCRIPTEN__) && defined(NDEBUG)
#define LogI(format, ...)
#define LogE(format, ...)
#else
#include <cstdio>
#define LogI(format, ...)                                                      \
    printf("info %s:%d " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define LogE(format, ...)                                                      \
    printf("error %s:%d " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#endif
