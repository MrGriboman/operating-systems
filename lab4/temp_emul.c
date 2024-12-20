#include <temp_logger.h>

#define USB_DEV_NAME "/dev/pts/3"
#define WRITE_PORT "COM3" // Replace with the write COM port
#define READ_PORT "COM5"  // Uncomment if you need to read data

int generate_temp()
{
    return rand() % 5 + 15;
}

#ifdef _WIN32
void get_local_time(char *buffer, size_t size)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, size, "%02d/%02d/%04d %02d:%02d:%02d",
             st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
}

HANDLE open_serial_port(const char *port_name)
{
    HANDLE hComm = CreateFileA(port_name,
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);

    if (hComm == INVALID_HANDLE_VALUE)
    {
        printf("Error opening port %s (error code: %ld)\n", port_name, GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);

    if (!GetCommState(hComm, &dcb))
    {
        printf("Error getting port state (error code: %ld)\n", GetLastError());
        CloseHandle(hComm);
        return INVALID_HANDLE_VALUE;
    }

    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;

    if (!SetCommState(hComm, &dcb))
    {
        printf("Error setting port state (error code: %ld)\n", GetLastError());
        CloseHandle(hComm);
        return INVALID_HANDLE_VALUE;
    }

    return hComm;
}

int write_port(HANDLE port, char *buffer, DWORD size)
{
    DWORD written;
    BOOL success = WriteFile(port, buffer, size, &written, NULL);
    if (!success)
    {
        printf("Failed to write to port\n");
        return -1;
    }
    if (written != size)
    {
        printf("Failed to write all bytes to port\n");
        return -1;
    }
    return 0;
}

#else
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
    dt->millisecond = roundf(ts.tv_nsec / 1000000);
}
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    srand((unsigned)time(NULL));

    HANDLE hWritePort = open_serial_port(WRITE_PORT);
    if (hWritePort == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    // HANDLE hReadPort = open_serial_port(READ_PORT); // Uncomment if reading
    // if (hReadPort == INVALID_HANDLE_VALUE)
    // {
    //     CloseHandle(hWritePort);
    //     return 1;
    // }

    char time_buffer[64];
    char output[128];

    while (1)
    {
        int temp = generate_temp();
        get_local_time(time_buffer, sizeof(time_buffer));
        snprintf(output, sizeof(output), "Time: %s Temp: %d\n", time_buffer, temp);

        printf("Writing to port: %s\n", output);
        write_port(hWritePort, output, strlen(output));

        // Uncomment to read from the other port

        // char read_buffer[128] = { 0 };
        // DWORD bytes_read;
        // if (ReadFile(hReadPort, read_buffer, sizeof(read_buffer) - 1, &bytes_read, NULL)) {
        //     printf("Received: %s\n", read_buffer);
        // } else {
        //     printf("Error reading from port (error code: %ld)\n", GetLastError());
        // }

        Sleep(1000); // 1 second delay
    }

    CloseHandle(hWritePort);
    // CloseHandle(hReadPort);

#else
    FILE *usb_dev;
    // FILE* usb_read;
    char str[256];
    usb_dev = fopen(USB_DEV_NAME, "a");
    // usb_read = fopen("/dev/pts/4", "r");
    if (usb_dev == NULL)
        perror("can't open file");
    DateTime dt;
    while (true)
    {
        int temp = generate_temp();
        get_local_time(&dt);
        fprintf(usb_dev, "Time: %d/%d/%d  %d:%d:%d Temp: %d\n", dt.day, dt.month, dt.year, dt.hour, dt.minute, dt.second, temp);
        printf("Temp measured\n");
        /*char *read_str = fgets(str, 256, usb_read);
        if (read_str == NULL)
            perror("error reading");
        else
            printf("%s", read_str);*/
        sleep(1);
    }
    fclose(usb_dev);
#endif
    return 0;
}