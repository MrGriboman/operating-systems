#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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


float generate_temp();
void get_local_time(DateTime *dt);