
#pragma warning(push, 0)

#ifdef WIN32_BUILD
#include <windows.h>
#include <GL/GL.h>

#define FRAMEBUFFER_FORMAT GL_RGBA16F
#endif

#ifdef ANDROID_BUILD
#include <GLES3/gl3.h>
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android/input.h>
#include <unistd.h>
#include <android/log.h>
#include <stdlib.h>
#include <pthread.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <time.h>

// currently opengles3 does not support floating point framebuffers
#define FRAMEBUFFER_FORMAT GL_RGBA
#endif

#ifdef __GNUC__
#define B_INLINE __attribute__((always_inline))
#else
#define B_INLINE __forceinline
#endif

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// REMOVE!!!
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG
#ifdef WIN32_BUILD
#define B_ASSERT(x) ((x) ? 0 : *((char *)0) = 'x')
#define DEBUG_DO(x) x
#elif defined(ANDROID_BUILD)
#define B_ASSERT(x) if (!(x)) __android_log_assert("Assertion Failed", "Branch", "%s: %d", __FILE__, __LINE__);
#define DEBUG_DO(x) x
#endif
#else
#define B_ASSERT(x)
#define DEBUG_DO(x)
#endif

#if defined(TIMERS) && defined(WIN32_BUILD)
#define BEGIN_TIME() u64 LOCAL_TIME = __rdtsc()
#define END_TIME() LOCAL_TIME = __rdtsc() - LOCAL_TIME
#define READ_TIME(into) into = LOCAL_TIME
#elif defined(TIMERS) && defined(ANDROID_BUILD)
#define BEGIN_TIME() timespec LOCAL_BEGIN_TIME, LOCAL_END_TIME;	\
   clock_gettime(CLOCK_MONOTONIC, &LOCAL_BEGIN_TIME)		
#define END_TIME() clock_gettime(CLOCK_MONOTONIC, &LOCAL_END_TIME);	\
   LONG LOCAL_TIME = LOCAL_END_TIME->tv_nsec - LOCAL_BEGIN_TIME->tv_nsec
   
#define READ_TIME(into) into = LOCAL_TIME
#else
#define BEGIN_TIME() 
#define END_TIME() 
#define READ_TIME(into)
#endif
#pragma warning(pop)

#include "log.h"
#include "branch_common.h"
#include "platformAPI.h"
#include "AssetHeader.h"
#include "Allocator.h"
#include "map.h"
#include "Renderer.h"
#include "Assets.h"

#include "main.h"

#ifdef ANDROID_BUILD
#include "Android/Branch_Android.h"
#include "Android/Android.cpp"
#endif

#ifdef WIN32_BUILD
#include "win32.h"
#include "win32.cpp"
#endif

#include "map.cpp"
#include "vertinc.h"
#include "Assets.cpp"
#include "Renderer.cpp"
#include "Gui.cpp"
#include "main.cpp"
