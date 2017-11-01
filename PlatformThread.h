/* PlatformThread.h */
/* Each platform must implement the platform thread API
 *
 * Each platform must provide these aliases
 * thread
 * sem
 * mutex
 * thread_return_type
 * thread_func
 *
 * Each platform must implement these functions/macros
 * CREATE_THREAD(thread)
 * SEM_WAIT(sem)
 * SEM_SIGNAL(sem)
 * MUTEX_LOCK(mutex)
 * MUTEX_UNLOCK(mutex)
 * SEM_DESTROY(sem)
 * MUTEX_DESTROY(mutex)
 */

#if defined(WIN32_BUILD)
typedef HANDLE thread;
typedef HANDLE sem;
typedef HANDLE mutex;
typedef DWORD thread_return_type;
typedef thread_return_type (*thread_func)(void *);

B_INLINE
HANDLE Win32CreateThread(thread_func func)
{
   // 4096 should be enough
   HANDLE result = CreateThread(0, 4096, func, 0, CREATE_SUSPENDED, 0);   
   B_ASSERT(result);
   ResumeThread(result);
   return result;
}

#define CREATE_THREAD(func) Win32CreateThread(func)
#define SEM_WAIT(sem) WaitForSingleObject(sem, INFINITE)
#define SEM_SIGNAL(sem) ReleaseSemaphore(sem, 1, 0)
#define SEM_DESTROY(sem) CloseHandle(sem)
#define MUTEX_LOCK(mut) WaitForSingleObject(mut, INFINITE)
#define MUTEX_UNLOCK(mut) ReleaseMutex(mut)
#define MUTEX_DESTROY(mut) CloseHandle(mut)

#elif defined(ANDROID_BUILD)
typedef pthread_t* thread;
typedef sem_t* sem;
typedef pthread_mutex_t* mutex;
typedef void* thread_return_type;
typedef thread_return_type (*thread_func)(void *);

B_INLINE
pthread_t AndroidCreateThread(thread_func func)
{
   pthread_t handle;
   int result = pthread_create(&handle, 0, func, 0);

   B_ASSERT(result == 0);
   return handle;
}

#define CREATE_THREAD(func) AndroidCreateThread(func)
#define SEM_WAIT(sem) sem_wait(sem)
#define SEM_SIGNAL(sem) sem_post(sem)
#define SEM_DESTROY(sem) sem_close(sem)
#define MUTEX_LOCK(mut) pthread_mutex_lock(mut)
#define MUTEX_UNLOCK(mut) pthread_mutex_unlock(mut)
#define MUTEX_DESTROY(mut) pthread_mutex_destroy(mut)
#endif
