/* include.cpp */
/* This is the main build file for the game
 * All source files are included here.
 */

#pragma warning(push, 0)

#ifdef WIN32_BUILD
#include <windows.h>
#include <GL/GL.h>
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
#include <semaphore.h>
#endif

#pragma warning(pop, 1)

#ifdef __GNUC__
#define B_INLINE __attribute__((always_inline))
#else
#define B_INLINE __forceinline
#endif

// Only used for getting character quads out of a font bitmap
// STB_TRUETYPE is used in the Asset Packing tool (ap)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// REMOVE!!!
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Debug and timers
#ifdef DEBUG
#define DEBUG_DO(expr) expr
#else
#define DEBUG_DO(expr)
#endif

#include "log.h"
#include "branch_common.h"
#include "PlatformAPI.h"
#include "PlatformThread.h"
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
#include "assets/vertinc.h"
#include "Assets.cpp"
#include "Renderer.cpp"
#include "Gui.cpp"
#include "main.cpp"
