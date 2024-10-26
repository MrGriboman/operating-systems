#ifndef BG_START
#define BG_START

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <signal.h>

#endif

#include <stdio.h>
#include <stdlib.h>

int create_bg_process(const char *program, char *const argv[], int wait);

#endif