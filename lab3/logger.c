#include <logger.h>

typedef struct
{
    char *file_name;
    int *counter;
} Log_struct;


#ifdef _WIN32
DWORD WINAPI increment(LPVOID param) {
    int* counter = (int*) param;
    while(1) {
        (*counter)++;
        printf("%d\n", *counter);
        Sleep(300);
    }
    return 0;
}


DWORD WINAPI log_to_file(LPVOID log_struct) {
    Log_struct* ls = (Log_struct*)(log_struct);
    char* file_name = ls->file_name;
    int* counter = ls->counter;


    SECURITY_ATTRIBUTES SA = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = 1
    };

    HANDLE file = CreateFileA(file_name, GENERIC_WRITE, FILE_SHARE_WRITE, &SA, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    do {
        DWORD pid = GetCurrentProcessId();
        SYSTEMTIME st;
        GetLocalTime(&st);
        char log[100];
        LPDWORD numberOfBytes = 0;
        sprintf(log, "PID: %lu, time: %d/%d/%d  %d:%d:%d %d\n", pid, st.wDay,st.wMonth,st.wYear, st.wHour, st.wMinute, st.wSecond , st.wMilliseconds);
        if (counter != NULL) {
            sprintf(log + strlen(log), "Current counter: %d\n", *(counter));
            Sleep(1000);
        }
        SetFilePointer(file, 0, NULL, FILE_END);
        WriteFile(file, log, strlen(log), numberOfBytes, NULL);
    } while (counter != NULL);
    CloseHandle(file);
    return 0;
}

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


pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;


void* increment(void* arg) {
    int* counter = (int*)arg;
    while (1) {
        pthread_mutex_lock(&data_mutex);
        (*counter)++; // Increment the counter
        printf("Counter: %d\n", *counter);
        pthread_mutex_unlock(&data_mutex);

        // Sleep for 300 milliseconds
        struct timespec req;
        req.tv_sec = 0;
        req.tv_nsec = 300 * 1000000; // 300 ms in nanoseconds
        nanosleep(&req, NULL);
    }
}


void get_local_time(DateTime* dt) {
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


void* log_to_file(void* log_struct) {
    Log_struct* ls = (Log_struct*)log_struct;
    char* file_name = ls->file_name;
    int* counter = ls->counter;
    DateTime dt;
    FILE* file;
    file = fopen(file_name, "a");

    do{
        printf("logging\n");
        int pid = getpid();
        get_local_time(&dt);

        fprintf(file, "PID: %d, time: %d/%d/%d  %d:%d:%d %d\n", pid, dt.day, dt.month, dt.year, dt.hour, dt.minute, dt.second , dt.millisecond);
        pthread_mutex_lock(&data_mutex);
        if (counter != NULL) {
            fprintf(file, "Current counter: %d\n", *counter);
        }
        fprintf(file, "\n");
        pthread_mutex_unlock(&data_mutex);
        fflush(file);
        sleep(1);
    } while (counter != NULL);
    fclose(file);

    return 0;
}
#endif


int main(int argc, char** argv) {
    char* file = argv[1];
    int counter = 0;
    Log_struct ls = {
        .file_name = file,
        .counter = NULL
    };
    log_to_file(&ls);
    ls.counter = &counter;

#ifdef _WIN32
    DWORD dwThreadIdArray[MAX_THREADS];
    HANDLE hThreadArray[MAX_THREADS];
    hThreadArray[0] = CreateThread(NULL, 0, increment, &counter, 0, &dwThreadIdArray[0]);
    if (hThreadArray[0] == NULL)
        printf("Error creating thread\n");

    hThreadArray[1] = CreateThread(NULL, 0, log_to_file, &ls, 0, &dwThreadIdArray[1]);
    if (hThreadArray[1] == NULL)
        printf("Error creating thread\n");
    while(1){}

#else
    pthread_t incr_thread, log_thread;
    int status_inc, status_log;
    status_inc = pthread_create(&incr_thread, NULL, increment, &counter);
    if (status_inc) 
        perror("Thread creation error!");
    
    status_log = pthread_create(&log_thread, NULL, log_to_file, &ls);
    if (status_log)
        perror("Thread creation error!");

#endif
    return 0;
}