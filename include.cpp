#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 640

#pragma warning(push, 0)

#include "platformAPI.h"

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

#pragma warning(pop)

#include "branch_common.h"
#include "main.h"

#include "map.cpp"
#include "vertinc.h"
#include "win32.h"
#include "main.cpp"

#ifdef WIN32_BUILD
#include "win32.cpp"
#endif
