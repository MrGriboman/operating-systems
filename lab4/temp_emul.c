#include <temp_logger.h>

#define USB_DEV_NAME "/dev/pts/3"



float generate_temp() {
    return rand() % 5 + 15;
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
    dt->millisecond = roundf(ts.tv_nsec / 1000000);
}


int main(int argc, char** argv) {
    
    FILE* usb_dev;
    //FILE* usb_read;
    char str[256];
    usb_dev = fopen(USB_DEV_NAME, "a");
    //usb_read = fopen("/dev/pts/4", "r");
    if (usb_dev == NULL)
        perror("can't open file");
    DateTime dt;
    while (true) {
        float temp = generate_temp();
        get_local_time(&dt);
        fprintf(usb_dev, "time: %d/%d/%d  %d:%d:%d %d, Temp: %f\n", dt.day, dt.month, dt.year, dt.hour, dt.minute, dt.second, dt.millisecond, temp);
        printf("Temp measured\n");
        /*char *read_str = fgets(str, 256, usb_read);
        if (read_str == NULL)
            perror("error reading");
        else
            printf("%s", read_str);*/
        sleep(1);
    }
    fclose(usb_dev);
    return 0;
}