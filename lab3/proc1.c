#include "logger.h"

#include <windows.h>
#include <stdio.h>

#define SHM_NAME "Local\\ShmCounter"
#define MUTEX_NAME "Global\\CounterMutex"

int main()
{
    HANDLE hMapFile;
    HANDLE hMutex;
    LPVOID pBuf;

    // Open the existing named mutex for synchronization
    hMutex = OpenMutex(
        MUTEX_ALL_ACCESS, // Desired access
        FALSE,            // Do not inherit the handle
        MUTEX_NAME);      // Mutex name

    if (hMutex == NULL)
    {
        printf("OpenMutex failed (%lu)\n", GetLastError());
        return 1;
    }

    // Open the existing shared memory region
    hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS, // Read/write access
        FALSE,               // Do not inherit the handle
        SHM_NAME);           // Name of the shared memory

    if (hMapFile == NULL)
    {
        printf("OpenFileMapping failed (%lu)\n", GetLastError());
        CloseHandle(hMutex);
        return 1;
    }

    // Map the shared memory into the current process's address space
    pBuf = MapViewOfFile(
        hMapFile,            // Handle to the map object
        FILE_MAP_ALL_ACCESS, // Read/write permission
        0,                   // High-order 32 bits of file offset
        0,                   // Low-order 32 bits of file offset
        256);                // Number of bytes to map

    if (pBuf == NULL)
    {
        printf("MapViewOfFile failed (%lu)\n", GetLastError());
        CloseHandle(hMapFile);
        CloseHandle(hMutex);
        return 1;
    }

    // Wait to acquire the mutex
    WaitForSingleObject(hMutex, INFINITE);

    // Access shared memory (write to it)
    int *counter = (int *)pBuf;
    //printf("Process 1: Incrementing counter from %d to %d\n", *counter, *counter + 10);
    *counter += 10;

    // Release the mutex
    ReleaseMutex(hMutex);

    // Clean up
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);

    return 0;
}