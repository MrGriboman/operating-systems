
#ifdef _WIN32
#include <windows.h>
#define MAX_THREADS 10
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <signal.h>
    #include <pthread.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <errno.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>


#ifdef _WIN32
DWORD WINAPI log_to_file(LPVOID log_stuct);
DWORD WINAPI increment(LPVOID param);
#else
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
} DateTime;

void* log_to_file(void* log_struct);
void* increment(void* arg);
void get_local_time(DateTime *dt);
#endif