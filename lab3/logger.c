#include <logger.h>

#define LOCK_FILE "./.lock"

typedef struct
{
    char *file_name;
    int *counter;
    char *info;
} Log_struct;

#ifdef _WIN32

#define SHM_NAME "Local\\ShmCounter"
#define COUNTER_MUTEX "Global\\CounterMutex"
#define SIZE_COUNTER sizeof(int)

// Synchronization primitives
CRITICAL_SECTION data_mutex;


// Function to get local time
void get_local_time(DateTime *dt)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    dt->year = st.wYear;
    dt->month = st.wMonth;
    dt->day = st.wDay;
    dt->hour = st.wHour;
    dt->minute = st.wMinute;
    dt->second = st.wSecond;
    dt->millisecond = st.wMilliseconds;
}

// Increment thread
DWORD WINAPI increment(LPVOID arg)
{
    int *counter = (int *)arg;
    while (1)
    {
        EnterCriticalSection(&data_mutex);
        (*counter)++;
        LeaveCriticalSection(&data_mutex);

        Sleep(300); // Sleep for 300 ms
    }
    return 0;
}

// Log thread
DWORD WINAPI log_to_file(LPVOID log_struct)
{
    Log_struct *ls = (Log_struct *)log_struct;
    HANDLE hFile = CreateFile(
        ls->file_name,         // Log file name
        FILE_APPEND_DATA,      // Open for writing and appending
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // Allow other processes to read and write
        NULL,                  // Default security attributes
        OPEN_ALWAYS,           // Open existing file or create new one
        FILE_ATTRIBUTE_NORMAL, // Normal file attributes
        NULL);                 // No template file

    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open log file. Error: %lu\n", GetLastError());
        return 1;
    }

    do
    {
        DateTime dt;
        get_local_time(&dt);

        char buffer[512]; // Buffer to hold the log message
        int length = snprintf(buffer, sizeof(buffer),
                              "PID: %lu, time: %d/%d/%d  %d:%d:%d %d\n",
                              GetCurrentProcessId(),
                              dt.day, dt.month, dt.year,
                              dt.hour, dt.minute, dt.second, dt.millisecond);

        EnterCriticalSection(&data_mutex);
        if (ls->counter != NULL)
        {
            length += snprintf(buffer + length, sizeof(buffer) - length,
                               "Current counter: %d\n", *(ls->counter));
        }
        if (ls->info != NULL)
        {
            length += snprintf(buffer + length, sizeof(buffer) - length,
                               "%s\n", ls->info);
        }
        LeaveCriticalSection(&data_mutex);

        // Write the log message to the file
        DWORD bytesWritten;
        if (!WriteFile(hFile, buffer, length, &bytesWritten, NULL))
        {
            fprintf(stderr, "Failed to write to log file. Error: %lu\n", GetLastError());
            CloseHandle(hFile);
            return 1;
        }

        // Ensure data is written to disk
        if (!FlushFileBuffers(hFile))
        {
            fprintf(stderr, "Failed to flush log file buffers. Error: %lu\n", GetLastError());
            CloseHandle(hFile);
            return 1;
        }

        Sleep(1000); // Log every second
    } while (ls->counter != NULL);

    CloseHandle(hFile);
    return 0;
}

// Input thread
DWORD WINAPI input_counter(LPVOID arg)
{
    int *counter = (int *)arg;
    char buf[100];
    while (1)
    {
        printf("Waiting for new value\n");
        fgets(buf, sizeof(buf), stdin);

        EnterCriticalSection(&data_mutex);
        printf("old: %d\n", *counter);
        *counter = atoi(buf);
        printf("new: %d\n", *counter);
        LeaveCriticalSection(&data_mutex);
    }
    return 0;
}

// Copy processes thread
DWORD WINAPI copy_process(LPVOID log_struct)
{
    Log_struct *ls = (Log_struct *)log_struct;
    bool finished_1 = true, finished_2 = true;
    PROCESS_INFORMATION procInfo1 = {0}, procInfo2 = {0};
    STARTUPINFO startInfo = {sizeof(STARTUPINFO)};

    while (1)
    {
        // Check or create the first process
        if (finished_1)
        {
            if (CreateProcess(
                    NULL,           // Application name (NULL if using command line)
                    "process1.exe", // Command line
                    NULL,           // Process security attributes
                    NULL,           // Thread security attributes
                    FALSE,          // Handle inheritance
                    0,              // Creation flags
                    NULL,           // Environment block
                    NULL,           // Current directory
                    &startInfo,     // Startup info
                    &procInfo1))    // Process information
            {
                finished_1 = false;             // Process 1 started
                CloseHandle(procInfo1.hThread); // Close thread handle
            }
            else
            {
                printf("Failed to create process 1: %lu\n", GetLastError());
            }
        }
        else
        {
            DWORD status = WaitForSingleObject(procInfo1.hProcess, 0);
            if (status == WAIT_OBJECT_0)
            {
                finished_1 = true;
                CloseHandle(procInfo1.hProcess); // Cleanup process handle
            }
            else
            {
                Log_struct log = {.file_name = ls->file_name, .counter = NULL, .info = "subprocess 1 hasn't finished yet"};
                log_to_file(&log);
            }
        }

        // Check or create the second process
        if (finished_2)
        {
            if (CreateProcess(
                    NULL,           // Application name (NULL if using command line)
                    "process2.exe", // Command line
                    NULL,           // Process security attributes
                    NULL,           // Thread security attributes
                    FALSE,          // Handle inheritance
                    0,              // Creation flags
                    NULL,           // Environment block
                    NULL,           // Current directory
                    &startInfo,     // Startup info
                    &procInfo2))    // Process information
            {
                finished_2 = false;             // Process 2 started
                CloseHandle(procInfo2.hThread); // Close thread handle
            }
            else
            {
                printf("Failed to create process 2: %lu\n", GetLastError());
            }
        }
        else
        {
            DWORD status = WaitForSingleObject(procInfo2.hProcess, 0);
            if (status == WAIT_OBJECT_0)
            {
                finished_2 = true;
                CloseHandle(procInfo2.hProcess); // Cleanup process handle
            }
            else
            {
                Log_struct log = {.file_name = ls->file_name, .counter = NULL, .info = "subprocess 2 hasn't finished yet"};
                log_to_file(&log);
            }
        }

        // Sleep before next iteration
        Sleep(3000);
    }

    return 0;
}

#else
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

void *increment(void *arg)
{
    int *counter = (int *)arg;
    while (1)
    {
        pthread_mutex_lock(&data_mutex);
        (*counter)++; // Increment the counter
        // printf("Counter: %d\n", *counter);
        pthread_mutex_unlock(&data_mutex);

        // Sleep for 300 milliseconds
        struct timespec req;
        req.tv_sec = 0;
        req.tv_nsec = 300 * 1000000; // 300 ms in nanoseconds
        nanosleep(&req, NULL);
    }
}

void get_local_time(DateTime *dt)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tm_info = localtime(&ts.tv_sec);

    dt->year = tm_info->tm_year + 1900;
    dt->month = tm_info->tm_mon + 1;
    dt->day = tm_info->tm_mday;
    dt->hour = tm_info->tm_hour;
    dt->minute = tm_info->tm_min;
    dt->second = tm_info->tm_sec;
    dt->millisecond = round(ts.tv_nsec / 1000000);
}

void *log_to_file(void *log_struct)
{
    Log_struct *ls = (Log_struct *)log_struct;
    char *file_name = ls->file_name;
    int *counter = ls->counter;
    DateTime dt;
    FILE *file;
    file = fopen(file_name, "a");

    do
    {
        // printf("logging\n");
        int pid = getpid();
        get_local_time(&dt);

        fprintf(file, "PID: %d, time: %d/%d/%d  %d:%d:%d %d\n", pid, dt.day, dt.month, dt.year, dt.hour, dt.minute, dt.second, dt.millisecond);
        pthread_mutex_lock(&data_mutex);
        if (counter != NULL)
        {
            fprintf(file, "Current counter: %d\n", *counter);
        }
        if (ls->info != NULL)
        {
            fprintf(file, "%s\n", ls->info);
        }
        fprintf(file, "\n");
        pthread_mutex_unlock(&data_mutex);
        fflush(file);
        sleep(1);
    } while (counter != NULL);
    fclose(file);

    return 0;
}

void *input_counter(void *counter)
{
    while (1)
    {
        int *counter_i = (int *)counter;
        char buf[100];
        printf("Waiting for new value\n");
        // sleep(5);
        fgets(buf, sizeof(buf), stdin);
        pthread_mutex_lock(&data_mutex);
        printf("old: %d\n", *counter_i);
        *counter_i = atoi(buf);
        printf("new: %d\n", *counter_i);
        pthread_mutex_unlock(&data_mutex);
        fflush(stdin);
    }
}

void *copy_process(void *log_struct)
{
    int status_1, status_2;
    bool finished_1 = true;
    bool finished_2 = true;
    pid_t pid_1 = -1, pid_2 = -1;

    while (1)
    {
        Log_struct *ls = (Log_struct *)log_struct;

        // Check or create the first process
        if (finished_1)
        {
            pid_1 = fork();
            if (pid_1 == -1)
            {
                perror("Failed to create the 1st process");
                break;
            }
            else if (pid_1 == 0)
            { // Child process 1
                Log_struct p1_log = {.file_name = ls->file_name, .counter = NULL, .info = "subprocess 1 starts"};
                log_to_file(&p1_log);

                pthread_mutex_lock(&data_mutex);
                printf("%d\n", *(ls->counter));
                *(ls->counter) += 10;
                printf("%d\n", *(ls->counter));
                pthread_mutex_unlock(&data_mutex);

                p1_log.info = "subprocess 1 ends";
                log_to_file(&p1_log);
                _exit(0);
            }
            else
            { // Parent process
                finished_1 = false; // Mark as unfinished since it has just started
            }
        }
        else
        { // Check if process 1 has finished
            if (waitpid(pid_1, &status_1, WNOHANG) > 0)
            {                
                finished_1 = true; // Mark as finished
            }
            else
            {
                Log_struct log = {.file_name = ls->file_name, .counter = NULL, .info = "subprocess 1 hasn't finished yet"};
                log_to_file(&log);
            }
        }

        // Check or create the second process
        if (finished_2)
        {
            pid_2 = fork();
            if (pid_2 == -1)
            {
                perror("Failed to create the 2nd process");
                break;
            }
            else if (pid_2 == 0)
            { // Child process 2
                Log_struct p2_log = {.file_name = ls->file_name, .counter = NULL, .info = "subprocess 2 starts"};
                log_to_file(&p2_log);

                pthread_mutex_lock(&data_mutex);
                *(ls->counter) *= 2;
                sleep(2);
                *(ls->counter) /= 2;
                pthread_mutex_unlock(&data_mutex);

                p2_log.info = "subprocess 2 ends";
                log_to_file(&p2_log);
                _exit(0);
            }
            else
            { // Parent process
                finished_2 = false; // Mark as unfinished since it has just started
            }
        }
        else
        { // Check if process 2 has finished
            if (waitpid(pid_2, &status_2, WNOHANG) > 0)
            {                
                finished_2 = true; // Mark as finished
            }
            else
            {
                Log_struct log = {.file_name = ls->file_name, .counter = NULL, .info = "subprocess 2 hasn't finished yet"};
                log_to_file(&log);
            }
        }

        // Sleep before next iteration
        sleep(3);
    }

    return 0;
}


#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    InitializeCriticalSection(&data_mutex);
#endif
    bool is_master = false;
    bool is_first_instance = false;
    char *file = "./log.txt";
    // int counter = 0;
    Log_struct ls = {
        .file_name = file,
        .counter = NULL,
        .info = NULL};
    log_to_file(&ls);
    // ls.counter = &counter;


#ifdef _WIN32
    HANDLE hMutex = CreateMutex(NULL, FALSE, "Global\\MyProgramMutex");
    HANDLE procMutex = CreateMutex(NULL, FALSE, COUNTER_MUTEX);

    if (hMutex == NULL)
    {
        printf("Error creating lock mutex: %ld\n", GetLastError());
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        printf("Another instance is running in master mode.\n");
        is_master = false;
    }
    else
    {
        printf("This instance is running in master mode.\n");
        is_master = true;
    }

    if (procMutex == NULL)
    {
        printf("Error creating process mutex: %ld\n", GetLastError());
        return 1;
    }

#else
    int fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        perror("Error opening lock file");
        exit(EXIT_FAILURE);
    }

    // Try to acquire an exclusive lock
    if (lockf(fd, F_TLOCK, 0) == -1)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            printf("Another instance detected. Running in restricted mode.\n");
        }
        else
        {
            perror("Error locking file");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        is_master = true; // This instance becomes the master
        printf("No other instances detected. Running in master mode.\n");

        // Write the process ID to the lock file
        char pid_str[10];
        snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
        write(fd, pid_str, strlen(pid_str));
    }
#endif


#ifdef _WIN32
        // Shared memory (file mapping)
        HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
        if (hMapFile == NULL)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                printf("Creating new shared memory.\n");
                hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SIZE_COUNTER, SHM_NAME);
                if (hMapFile == NULL)
                {
                    printf("Failed to create shared memory: %lu\n", GetLastError());
                    return EXIT_FAILURE;
                }
                is_first_instance = true;
            }
            else
            {
                printf("Failed to open shared memory: %lu\n", GetLastError());
                return EXIT_FAILURE;
            }
    }

    int *counter_shm = (int *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SIZE_COUNTER);
    if (counter_shm == NULL)
    {
        printf("Failed to map shared memory: %lu\n", GetLastError());
        CloseHandle(hMapFile);
        return EXIT_FAILURE;
    }

    if (is_first_instance)
    {
        *counter_shm = 0;
        printf("Initialized shared memory.\n");
    }

#else
    const char *shm_name = "/shmem_counter";
    const int SIZE_COUNTER = sizeof(int);

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        if (errno == ENOENT)
        {
            // Shared memory doesn't exist
            printf("creating new shmem\n");
            shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1)
            {
                perror("Failed to open existing shared memory or create new one");
                exit(1);
            }
            if (ftruncate(shm_fd, SIZE_COUNTER) == -1)
            {
                perror("ftruncate failed\n");
            }
            is_first_instance = true;
        }
        else
        {
            perror("shm_open failed");
            exit(1);
        }
    }

    int *counter_shm = mmap(0, SIZE_COUNTER, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (counter_shm == MAP_FAILED)
    {
        perror("mmap failed\n");
    }

    if (is_first_instance)
    {
        *counter_shm = 0;
        printf("initialized shm\n");
    }

#endif

#ifdef _WIN32
    ls.counter = counter_shm;
    // Create threads
    int NThreads = (is_master) ? 4 : 2;
    HANDLE hThreads[NThreads];
    DWORD threadIds[NThreads];

    hThreads[0] = CreateThread(NULL, 0, increment, ls.counter, 0, &threadIds[0]);
    if (hThreads[0] == NULL)
    {
        printf("Error creating increment thread: %lu\n", GetLastError());
    }

    hThreads[1] = CreateThread(NULL, 0, input_counter, ls.counter, 0, &threadIds[2]);
    if (hThreads[1] == NULL)
    {
        printf("Error creating input thread: %lu\n", GetLastError());
    }

    if (is_master)
    {
        hThreads[2] = CreateThread(NULL, 0, log_to_file, &ls, 0, &threadIds[1]);
        if (hThreads[2] == NULL)
        {
            printf("Error creating log thread: %lu\n", GetLastError());
        }

        hThreads[3] = CreateThread(NULL, 0, copy_process, &ls, 0, &threadIds[3]);
        if (hThreads[3] == NULL)
        {
            printf("Error creating copy process thread: %lu\n", GetLastError());
        }
    }

    // Wait for threads to finish
    WaitForMultipleObjects(NThreads, hThreads, TRUE, INFINITE);

    // Cleanup
    for (int i = 0; i < NThreads ; i++)
    {
        if (hThreads[i])
            CloseHandle(hThreads[i]);
    }

    UnmapViewOfFile(counter_shm);
    CloseHandle(hMapFile);

    DeleteCriticalSection(&data_mutex);

#else
    ls.counter = counter_shm;
    pthread_t incr_thread, log_thread, input_thread, copy_thread;
    int status_inc, status_log, status_input, status_copy;

    status_inc = pthread_create(&incr_thread, NULL, increment, ls.counter);
    if (status_inc)
        perror("Thread creation error!");

    if (is_master)
    {
        status_log = pthread_create(&log_thread, NULL, log_to_file, &ls);
        if (status_log)
            perror("Thread creation error!");
    }

    status_input = pthread_create(&input_thread, NULL, input_counter, ls.counter);
    if (status_input)
        perror("Thread creation errorr");

    if (is_master)
    {
        status_copy = pthread_create(&copy_thread, NULL, copy_process, &ls);
        if (status_copy)
            perror("Thread creation errorr");
    }

    pthread_join(input_thread, NULL);
    pthread_join(incr_thread, NULL);
    pthread_join(copy_thread, NULL);
    pthread_join(log_thread, NULL);

    if (munmap(counter_shm, SIZE_COUNTER) == -1)
    {
        perror("munmap failed\n");
    }
    close(shm_fd);

    // Uncomment the line below to remove shared memory after testing
    shm_unlink(shm_name);
#endif
    return 0;
}