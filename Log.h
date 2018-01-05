
// Dumb simple stdio log for debug

#ifdef DEBUG
#ifdef WIN32_BUILD
static FILE *logFile;
#define LOG_WRITE(...) LogWrite(0, __VA_ARGS__)
#define INIT_LOG() InitLog();

#pragma warning(push, 0)
void LogWrite(int what, ...)
{
   va_list args;
   va_start(args, what);
   fprintf(logFile, args);
   fflush(logFile);
}
#pragma warning(pop)

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
