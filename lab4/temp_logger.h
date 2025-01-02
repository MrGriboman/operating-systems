#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#else

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <termios.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#endif

typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
} DateTime;

typedef struct
{
    char date[20];
    float temp;
} TempInfo;


int generate_temp();
void clean_log_file(FILE *log_file, struct tm *lt, const char *file_name);

#ifdef _WIN32
    void get_local_time(char *buffer, size_t size);
HANDLE open_serial_port(const char *port_name);
int write_port(HANDLE port, char *buffer, DWORD size);
HANDLE open_com_port(const char *port_name);
DWORD read_com_port(HANDLE port, char *buffer, size_t size);
void flush_incomplete_line(HANDLE hComm);
#else
    void get_local_time(DateTime *dt);
void flush_incomplete_line(FILE *usb_read)
#endif