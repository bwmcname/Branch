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
#include <fcntl.h>
#include <sys/stat.h>
#include <android/native_activity.h>
#include <android/window.h>
#include <android/configuration.h>
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
#include <jni.h>

#if !defined(CUSTOM_BUILD)
#include <firebase/admob.h>
#include <firebase/admob/types.h>
#include <firebase/app.h>
#include <firebase/future.h>
#endif
#endif

#pragma warning(pop)

#ifdef __GNUC__
#define B_INLINE __attribute__((always_inline))
#else
#define B_INLINE __forceinline
#endif

// Only used for getting character quads out of a font bitmap
// STB_TRUETYPE is used in the Asset Packing tool (ap)

// @REMOVE IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

// @REMOVE
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Debug and timers
#ifdef DEBUG
#define DEBUG_DO(expr) expr
#else
#define DEBUG_DO(expr)
#endif

#include "BranchTypes.h"
#include "Log.h"
#include "BranchMath.h"
#include "BranchCommon.h"
#include "PlatformAPI.h"
#include "BranchOpengl.h"
#include "PlatformThread.h"
#include "AssetHeader.h"
#include "Allocator.h"
#include "Map.h"
#include "Renderer.h"
#include "Assets.h"
#include "assets/GUIMap.h"

#ifdef ANDROID_BUILD
#include "Android/Branch_Android.h"
#endif


#include "Main.h"

//@TEMPORARY!!!
void LoadGLState(GameState &state, StackAllocator *stack, AssetManager &assetManager);

#ifdef ANDROID_BUILD
#include "Android/Android.cpp"
#endif

#ifdef WIN32_BUILD
#include "Win32.h"
#include "Win32.cpp"
#endif

#include "Map.cpp"
#include "assets/vertinc.h"
#include "Assets.cpp"
#include "Renderer.cpp"
#include "BranchOpengl.cpp"
#include "Gui.cpp"
#include "Main.cpp"
