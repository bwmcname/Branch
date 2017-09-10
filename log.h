
// Dumb simple stdio log for debug

#ifdef DEBUG
static FILE *logFile;
#define LOG_WRITE(...) fprintf(logFile, __VA_ARGS__)
#define INIT_LOG() InitLog();

void InitLog()
{
   logFile = fopen("log.txt", "wb");

   assert(logFile);
}

#else
#define LOG_WRITE(...)
#define INIT_LOG()
#endif
