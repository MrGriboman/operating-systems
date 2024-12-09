// socat -d -d pty,raw,echo=0 pty,raw,echo=0

#include <temp_logger.h>

#define USB_DEV_READ "/dev/pts/4"

int main()
{
    FILE *usb_read;
    usb_read = fopen(USB_DEV_READ, "r");
    if (usb_read == NULL)
        perror("Couldn't open usb\n");

    FILE *log_file;
    log_file = fopen("./log.txt", "a");
    if (log_file == NULL)
        perror("Couldn't open the log file\n");

    FILE *avg_hour;
    avg_hour = fopen("./avg_hour.txt", "a");
    if (avg_hour == NULL)
        perror("Couldn't open avg_hour file\n");

    char str[256];
    while (true)
    {
        char *read_str = fgets(str, 256, usb_read);
        if (read_str == NULL)
            continue;
        else
            //printf("%s\n", read_str); 
            fprintf(log_file, "%s\n", read_str);
            fflush(log_file);
    }
    fclose(usb_read);
    fclose(log_file);
    return 0;
}