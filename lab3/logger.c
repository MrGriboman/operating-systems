#include <logger.h>

/*int log_to_file(const char* file_name) {
    FILE* file;
    file = fopen(file_name, "a");
    DWORD pid = GetCurrentProcessId();
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(file, "PID: %lu, time: %d/%d/%d  %d:%d:%d %d\n", pid, st.wDay,st.wMonth,st.wYear, st.wHour, st.wMinute, st.wSecond , st.wMilliseconds);
    fclose(file);
}*/

int log_to_file(const char* file_name, int counter) {
    SECURITY_ATTRIBUTES SA = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = 1
    };

    HANDLE file = CreateFileA(file_name, GENERIC_WRITE, FILE_SHARE_WRITE, &SA, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    DWORD pid = GetCurrentProcessId();
    SYSTEMTIME st;
    GetLocalTime(&st);
    char log[100];
    LPDWORD numberOfBytes = 0;
    sprintf(log, "PID: %lu, time: %d/%d/%d  %d:%d:%d %d\n", pid, st.wDay,st.wMonth,st.wYear, st.wHour, st.wMinute, st.wSecond , st.wMilliseconds);
    SetFilePointer(file, 0, NULL, FILE_END);
    WriteFile(file, log, strlen(log), numberOfBytes, NULL);
    CloseHandle(file);
    return 0;
}


int main() {
    const char* file = "./test.txt";
    log_to_file(file, 0);
    return 0;
}