#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 640

#pragma warning(push, 0)

#ifdef WIN32_BUILD
#include <windows.h>
#include <GL/GL.h>
#endif

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <math.h>
#include <stdlib.h>

#ifdef DEBUG
#define assert(x) ((x) ? 0 : *((char *)0) = 'x')
#define DEBUG_DO(x) x
#else
#define assert(x)
#define DEBUG_DO(x)
#endif

#ifdef TIMERS
#define BEGIN_TIME() u64 LOCAL_TIME = __rdtsc()
#define END_TIME() LOCAL_TIME = __rdtsc() - LOCAL_TIME
#define READ_TIME(into) into = LOCAL_TIME
#else
#define BEGIN_TIME() 
#define END_TIME() 
#define READ_TIME(into)
#endif

#pragma warning(pop)

#include "branch_common.h"
#include "platformAPI.h"
#include "AssetHeader.h"
#include "Allocator.h"
#include "map.h"
#include "renderer.h"
#include "Assets.h"

#ifdef WIN32_BUILD
#include "win32.h"
#include "main.h"
#include "win32.cpp"
#endif

#include "map.cpp"
#include "vertinc.h"
#include "Assets.cpp"
#include "Renderer.cpp"
#include "main.cpp"
