/*

// Needs to be initialized!!!
AAssetManager *assetManager;

// @implement!!!
static inline
u64 bclock()
{
   return 0x67123C4AA8F234AD;
}

static inline
AAsset *AndroidFileOpen(char *filename)
{
   return AAssetManager_open(assetManager, filename, AASSET_MODE_STREAMING);
}

static inline
size_t AndroidFileSize(char *filename)
{
   AAsset *file = AAssetManager_open(assetManager, filename, AASSET_MODE_UNKNOWN);
   return AAsset_getLength64(file);
}

static inline
u8 *AndroidAllocateSystemMemory(size_t size, size_t *outSize)
{
   // return enough memory pages to fit size
   int pageSize = getpagesize();
   int pages;

   if(size == pageSize)
   {
      pages = 1;
   }
   else
   {
      pages = (size / pageSize) + 1;
   }

   return (u8 *)malloc(pages * pageSize);
}


extern
void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t stateSize)
{
   for(;;)
   {
      __android_log_print(ANDROID_LOG_DEBUG, "wow: ", "This is a message from Brian");
   }
}
*/

// Needs to be initialized!!!
AAssetManager *assetManager;

// @implement!!!
static inline
u64 bclock()
{
   return 0x67123C4AA8F234AD;
}

static inline
AAsset *AndroidFileOpen(char *filename)
{
   return AAssetManager_open(assetManager, filename, AASSET_MODE_STREAMING);
}

static inline
size_t AndroidFileSize(char *filename)
{
   AAsset *file = AAssetManager_open(assetManager, filename, AASSET_MODE_UNKNOWN);
   return AAsset_getLength64(file);
}

static inline
u8 *AndroidAllocateSystemMemory(size_t size, size_t *outSize)
{
   // return enough memory pages to fit size
   int pageSize = getpagesize();
   int pages;

   if(size == pageSize)
   {
      pages = 1;
   }
   else
   {
      pages = (size / pageSize) + 1;
   }

   return (u8 *)malloc(pages * pageSize);
}

void android_main(android_app *app)
{
   for(;;);
   {
      __android_log_print(ANDROID_LOG_DEBUG, "wow: ", "This is a message from Brian");
   }
}
