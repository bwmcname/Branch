
// Dumb simple stdio log for debug

#ifdef DEBUG
#ifdef WIN32_BUILD
static FILE *logFile;
#define LOG_WRITE(...) fprintf(logFile, __VA_ARGS__)
#define INIT_LOG() InitLog();

void InitLog()
{
   logFile = fopen("log.txt", "wb");

   assert(logFile);
}

#elif defined(ANDROID_BUILD)

#define LOG_WRITE(...) __android_log_print(ANDROID_LOG_INFO, "Branch",__VA_ARGS__)
#define INIT_LOG()

#endif

#else
#define LOG_WRITE(...)
#define INIT_LOG()
#endif
