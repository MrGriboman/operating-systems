#include <logger.h>

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

    // Access shared memory (perform calculations)
    int *counter = (int *)pBuf;
   // printf("Process 2: Performing operation on counter %d\n", *counter);

    // Simulate the operation (multiply and divide)
    *counter *= 2;
    Sleep(2000); // Simulate some time for the operation
    *counter /= 2;

   // printf("Process 2: Counter after operation: %d\n", *counter);

    // Release the mutex
    ReleaseMutex(hMutex);

    // Clean up
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);

    return 0;
}