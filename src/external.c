#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push, 0)
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <Empire/stb_image.h>

#define STB_DS_IMPLEMENTATION
#include <Empire/stb_ds.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <Empire/stb_truetype.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#include <Empire/miniaudio.h>

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
