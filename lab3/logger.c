#include <logger.h>

typedef struct
{
    char *file_name;
    int *counter;
    char *info;
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
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_FILE "./.lock"


void* increment(void* arg) {
    int* counter = (int*)arg;
    while (1) {
        pthread_mutex_lock(&data_mutex);
        (*counter)++; // Increment the counter
        //printf("Counter: %d\n", *counter);
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
        //printf("logging\n");
        int pid = getpid();
        get_local_time(&dt);

        fprintf(file, "PID: %d, time: %d/%d/%d  %d:%d:%d %d\n", pid, dt.day, dt.month, dt.year, dt.hour, dt.minute, dt.second , dt.millisecond);
        pthread_mutex_lock(&data_mutex);
        if (counter != NULL) {
            fprintf(file, "Current counter: %d\n", *counter);
        }
        if (ls->info != NULL) {
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


void* input_counter(void* counter) {
    while (1) {
        int* counter_i = (int*)counter;
        char buf[100];
        printf("Waiting for new value\n");
        //sleep(5);
        fgets(buf, sizeof(buf), stdin);
        pthread_mutex_lock(&data_mutex);
        printf("old: %d\n", *counter_i);
        *counter_i = atoi(buf);
        printf("new: %d\n", *counter_i);
        pthread_mutex_unlock(&data_mutex);
        fflush(stdin);
    }
}


void* copy_process(void* log_struct) {
    while (1) {
        Log_struct* ls = (Log_struct*)log_struct;
        pid_t pid_1, pid_2;
        switch (pid_1 = fork())
        {
        case -1:
            perror("Failed to create the 1st process");
            break;
        
        case 0:
            Log_struct p1_log = {.file_name = ls->file_name, .counter = NULL, .info = "process 1 starts"};
            log_to_file(&p1_log);
            pthread_mutex_lock(&data_mutex);
            //printf("old: %d\n", *(ls->counter));
            *(ls->counter) += 10;
            //printf("new: %d\n", *(ls->counter));
            pthread_mutex_unlock(&data_mutex);
            p1_log.info = "process 1 ends";
            log_to_file(&p1_log);  
            int ex_status = 0;   
            _exit(ex_status);       
        default:
            switch (pid_2 = fork())
            {
            case -1:
                perror("Failed to create the 2nd process");
                break;
            
            case 0:
                Log_struct p2_log = {.file_name = ls->file_name, .counter = NULL, .info = "process 2 starts"};
                log_to_file(&p2_log);
                pthread_mutex_lock(&data_mutex);                
                *(ls->counter) *= 2;
                sleep(2);
                *(ls->counter) /= 2;
                pthread_mutex_unlock(&data_mutex);
                p2_log.info = "process 2 ends";
                log_to_file(&p2_log);  
                int ex_status = 0;   
                _exit(ex_status);    
                break;
            default:
                int status_1, status_2;
                waitpid(pid_1, &status_1, WNOHANG);
                waitpid(pid_2, &status_2, WNOHANG);
                
                break;
            }
            break;
        }
        sleep(3);
    }
}
#endif


int main(int argc, char** argv) {
    bool is_master = false;
    bool is_first_instance = false;
    char* file = "./log.txt";
    //int counter = 0;
    Log_struct ls = {
        .file_name = file,
        .counter = NULL,
        .info = NULL
    };
    log_to_file(&ls);
    //ls.counter = &counter;

    int fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("Error opening lock file");
        exit(EXIT_FAILURE);
    }

    // Try to acquire an exclusive lock
    if (lockf(fd, F_TLOCK, 0) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            printf("Another instance detected. Running in restricted mode.\n");
        } else {
            perror("Error locking file");
            exit(EXIT_FAILURE);
        }
    } else {
        is_master = true; // This instance becomes the master
        printf("No other instances detected. Running in master mode.\n");

        // Write the process ID to the lock file
        char pid_str[10];
        snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
        write(fd, pid_str, strlen(pid_str));
    }

    const char *shm_name = "/shmem_counter";
    const int SIZE_COUNTER = sizeof(int);

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        if (errno == ENOENT) {
            // Shared memory doesn't exist
            printf("creating new shmem\n");
            shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1) {
                perror("Failed to open existing shared memory or create new one");
                exit(1);
            }
            if (ftruncate(shm_fd, SIZE_COUNTER) == -1) {
                perror("ftruncate failed\n");
            }
            is_first_instance = true;
        } 
        else {
            perror("shm_open failed");
            exit(1);
        }
    }

    

    int *counter_shm = mmap(0, SIZE_COUNTER, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (counter_shm == MAP_FAILED) {
        perror("mmap failed\n");
    }

    if (is_first_instance) {
        *counter_shm = 0;
        printf("initialized shm\n");
    }

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
    ls.counter = counter_shm;
    pthread_t incr_thread, log_thread, input_thread, copy_thread;
    int status_inc, status_log, status_input, status_copy;

    status_inc = pthread_create(&incr_thread, NULL, increment, ls.counter);
    if (status_inc) 
        perror("Thread creation error!");
    
    if (is_master) {
        status_log = pthread_create(&log_thread, NULL, log_to_file, &ls);
        if (status_log)
            perror("Thread creation error!");
    }

    status_input = pthread_create(&input_thread, NULL, input_counter, ls.counter);
    if (status_input)
        perror("Thread creation errorr");

    if (is_master) {
        status_copy = pthread_create(&copy_thread, NULL, copy_process, &ls);
        if (status_copy)
            perror("Thread creation errorr");
    }

    pthread_join(input_thread, NULL);
    pthread_join(incr_thread, NULL);
    pthread_join(copy_thread, NULL);
    pthread_join(log_thread, NULL);
    

    if(munmap(counter_shm, SIZE_COUNTER) == -1) {
        perror("munmap failed\n");
    }
    close(shm_fd);

    // Uncomment the line below to remove shared memory after testing
    shm_unlink(shm_name);
#endif
    return 0;
}